"""Configure-only regressions; run with Python 3, CMake, and a C++ compiler.

No ROCm SDK or GPU is needed. Check missing utilities and target selection
before SDK discovery.
"""
import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class ConfigureRegressions(unittest.TestCase):
    source = pathlib.Path(__file__).resolve().parents[2]

    def configure(self, directory, *options, standalone=False):
        source = self.source / "videoDecode" if standalone else self.source
        compiler = os.environ.get("ROCPYDECODE_TEST_CXX") or shutil.which("c++")
        self.assertIsNotNone(compiler, "A C++ compiler is required")
        result = subprocess.run(
            [os.environ.get("ROCPYDECODE_TEST_CMAKE", "cmake"), "-S", str(source), "-B", str(directory / "build"),
             "-DCMAKE_CXX_COMPILER=" + compiler,
             "-DROCM_PATH=" + str(directory / "no-sdk"),
             "-DBUILD_TESTING=OFF", *options],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertNotIn("CMake Error", result.stdout)
        self.assertIn("nothing will be built", result.stdout)
        for install in (directory / "build").rglob("cmake_install.cmake"):
            self.assertNotIn("pyRocVideoDecode", install.read_text())
        return result.stdout

    def test_missing_default_directory(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = pathlib.Path(temporary)
            output = self.configure(directory, "-DBUILD_JPEG_DECODE=OFF")
            self.assertIn("Skipping rocPyVideoDecode", output)
            self.assertIn("roc_video_dec.cpp", output)

    def test_explicit_incomplete_directory_is_authoritative(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = pathlib.Path(temporary)
            utilities = directory / "external utilities"
            utilities.mkdir()
            # A directory with a required file's name is not a usable source file.
            (utilities / "colorspace_kernels.cpp").mkdir()
            output = self.configure(
                directory, "-DBUILD_JPEG_DECODE=OFF",
                "-DROCPYVIDEO_UTILS_DIR=" + str(utilities),
            )
            self.assertIn(str(utilities), output)
            self.assertIn("colorspace_kernels.cpp", output)
            self.assertIn("No alternative directory will be searched", " ".join(output.split()))

    def test_standalone_missing_utilities(self):
        with tempfile.TemporaryDirectory() as temporary:
            self.configure(pathlib.Path(temporary), standalone=True)

    def test_explicitly_disabled_video_skips_validation(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = self.configure(
                pathlib.Path(temporary), "-DBUILD_VIDEO_DECODE=OFF",
                "-DBUILD_JPEG_DECODE=OFF",
            )
            self.assertNotIn("Skipping rocPyVideoDecode", output)

    def test_gpu_target_selection(self):
        defaults = (
            "gfx950;gfx942;gfx90a;gfx908;gfx1250;gfx1201;gfx1200;gfx1153;gfx1152;"
            "gfx1151;gfx1150;gfx1103;gfx1102;gfx1101;gfx1100;gfx1036;gfx1035;"
            "gfx1034;gfx1033;gfx1032;gfx1031;gfx1030;gfx1012;gfx1011;gfx1010"
        )
        compiler = os.environ.get("ROCPYDECODE_TEST_CXX") or shutil.which("c++")
        self.assertIsNotNone(compiler, "A C++ compiler is required")
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            modules = root / "modules"
            modules.mkdir()
            # Capture target selection at HIP discovery and stop before SDK use.
            (modules / "FindHIP.cmake").write_text(
                'file(WRITE "${CMAKE_BINARY_DIR}/targets.txt" '
                '"${GPU_TARGETS}\\n${GPU_BUILD_TARGETS}")\n'
                'message(FATAL_ERROR "GPU_TARGET_PROBE_COMPLETE")\n'
            )
            for name in (
                "rocvideodecode/roc_video_dec.cpp", "rocvideodecode/roc_video_dec.h",
                "colorspace_kernels.cpp", "colorspace_kernels.h", "resize_kernels.h",
                "video_post_process.h",
            ):
                path = root / "sdk/share/rocdecode/utils" / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.touch()

            def probe(source, build, expected, *options):
                observed = build / "targets.txt"
                if observed.exists():
                    observed.unlink()
                result = subprocess.run(
                    [os.environ.get("ROCPYDECODE_TEST_CMAKE", "cmake"),
                     "-S", str(source), "-B", str(build),
                     "-DCMAKE_CXX_COMPILER=" + compiler,
                     "-DCMAKE_MODULE_PATH=" + str(modules),
                     "-DROCM_PATH=" + str(root / "sdk"),
                     "-DBUILD_TESTING=OFF", *options],
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                    text=True, timeout=60,
                )
                self.assertNotEqual(result.returncode, 0, result.stdout)
                if expected is None:
                    self.assertIn("GPU_TARGETS must contain at least one", result.stdout)
                    self.assertFalse(observed.exists(), result.stdout)
                else:
                    self.assertIn("GPU_TARGET_PROBE_COMPLETE", result.stdout)
                    self.assertEqual(observed.read_text().splitlines(), [expected, expected])

            for label, source, options in (
                ("root", self.source, ()),
                ("root-jpeg", self.source, ("-DBUILD_VIDEO_DECODE=OFF",)),
                ("video", self.source / "videoDecode", ()),
                ("jpeg", self.source / "jpegDecode", ()),
            ):
                with self.subTest(build=label):
                    build = root / label
                    probe(source, build, defaults, *options)
                    probe(source, build, "gfx942", *options, "-DGPU_TARGETS=gfx942")
                    probe(source, build, "gfx942", *options)
                    probe(source, build, "gfx90a;gfx1100", *options,
                          "-DGPU_TARGETS=gfx90a;gfx1100")
                    probe(source, root / (label + "-legacy"), "gfx942", *options,
                          "-DAMDGPU_TARGETS=gfx942")
                    probe(source, root / (label + "-precedence"), "gfx1100", *options,
                          "-DGPU_TARGETS=gfx1100", "-DAMDGPU_TARGETS=gfx942")
                    probe(source, root / (label + "-empty"), None, *options,
                          "-DGPU_TARGETS=")
                    # Model an older build cache, including a stale derived list.
                    migrated = root / (label + "-migrated")
                    migrated.mkdir()
                    (migrated / "CMakeCache.txt").write_text(
                        "GPU_TARGETS:STRING=gfx906\nGPU_BUILD_TARGETS:STRING=gfx942\n")
                    probe(source, migrated, "gfx906", *options)
                    probe(source, migrated, "gfx1100", *options, "-DGPU_TARGETS=gfx1100")


if __name__ == "__main__":
    unittest.main()
