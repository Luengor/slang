#!/bin/env python3
import sys, os
from dataclasses import dataclass
from enum import Enum
import subprocess

SLANG_EXTENSIONS = [".slang", ".sl"]

# Check command line arguments
if len(sys.argv) != 3:
    print("Usage: python3 main.py <executable> <test_directory>")
    sys.exit(1)

executable = sys.argv[1]
test_directory = sys.argv[2]

# Find all tests
class TestStatus(Enum):
    PASS = "PASS"
    FAIL = "FAIL"
    CRASH = "CRASH"

@dataclass
class TestResult:
    status: TestStatus
    details: str = ""

@dataclass
class TestCase:
    case_name: str
    groups: list[str]
    input_file: str
    expected_output_file: str

    @staticmethod
    def from_path(path: str, base_path:str = "") -> 'TestCase':
        rel_path = path if base_path == "" else os.path.relpath(path, base_path)
        parts = rel_path.split(os.sep)
        name = os.path.splitext(parts[-1])[0]
        groups = parts[:-1]
        input_file = path

        with open(path, 'r') as f:
            lines = f.readlines()

        # First line is '# <expected output>'
        expected_output = lines[0].strip().strip('# ').strip()
        return TestCase(
            case_name=name,
            groups=groups,
            input_file=input_file,
            expected_output_file=expected_output
        )

    def full_name(self) -> str:
        return '.'.join(self.groups + [self.case_name])

    def test(self) -> TestResult:
        try:
            result = subprocess.run([executable, self.input_file], capture_output=True, text=True, timeout=5)
            output = result.stdout.strip()
            if output == self.expected_output_file:
                return TestResult(status=TestStatus.PASS)
            else:
                return TestResult(status=TestStatus.FAIL, details=f"Expected: {self.expected_output_file}, Got: {output}")
        except Exception as e:
            return TestResult(status=TestStatus.CRASH, details=str(e))

    def __str__(self) -> str:
        return f"TestCase(name={self.case_name}, groups={self.groups}, input_file={self.input_file}, expected_output_file={self.expected_output_file})"

test_cases: list[TestCase] = []
for root, dirs, files in os.walk(test_directory):
    for file in files:
        if not any(file.endswith(ext) for ext in SLANG_EXTENSIONS): continue

        test_cases.append(TestCase.from_path(os.path.join(root, file), test_directory))

print(f"Found {len(test_cases)} test cases.")

# Run tests
print("Running tests...")
counts = {
    TestStatus.PASS: 0,
    TestStatus.FAIL: 0,
    TestStatus.CRASH: 0
}
for test_case in test_cases:
    print(f"{test_case.full_name()}: ", end="")
    result = test_case.test()

    if result.status == TestStatus.PASS:
        print("\033[92mPASS\033[0m")
    elif result.status == TestStatus.FAIL:
        print("\033[91mFAIL\033[0m")
        print(f"  Details: {result.details}")
    else:
        print("\033[93mCRASH\033[0m")
        print(f"  Details: {result.details}")
    counts[result.status] += 1

# Summary
print("\nTest Summary:")
total = sum(counts.values())
for status, count in counts.items():
    print(f"{status.value}: {count} ({(count/total*100) if total > 0 else 0:.2f}%)")
