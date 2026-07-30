import importlib
import sys


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: import_module_test.py <module>")

    module_name = sys.argv[1]
    module = importlib.import_module(module_name)
    print(f"{module_name}: {module.__file__}")


if __name__ == "__main__":
    main()
