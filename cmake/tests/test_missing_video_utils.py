"""Configure-only regressions; run with Python 3, CMake, and a C++ compiler.

No ROCm SDK is needed: incomplete utilities must skip video before SDK discovery.
"""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class MissingVideoUtilities(unittest.TestCase):
    source = pathlib.Path(__file__).resolve().parents[2]

    def configure(self, directory, *options, standalone=False):
        source = self.source / "videoDecode" if standalone else self.source
        compiler = shutil.which("c++")
        self.assertIsNotNone(compiler, "A C++ compiler is required")
        result = subprocess.run(
            ["cmake", "-S", str(source), "-B", str(directory / "build"),
             "-DCMAKE_CXX_COMPILER=" + compiler,
             "-DROCM_PATH=" + str(directory / "no-sdk"), *options],
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


if __name__ == "__main__":
    unittest.main()
