#!/bin/env python3
import os
from dataclasses import dataclass
from enum import Enum
import subprocess

SLANG_EXTENSIONS = [".slang", ".sl"]
COMPERROR = "COMPERROR"
COMPERROR_CODE = 65

# Make the project in the root test_directory
root_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(root_dir)

print("Building the project...")
result = subprocess.run(["make"])
if result.returncode != 0:
    print("Build failed. Exiting.")
    exit(1)
executable = os.path.join(root_dir, "slang")

# Find all tests
print("Discovering test cases...")

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
    expected_output: str

    @staticmethod
    def from_path(path: str, base_path:str = "") -> 'TestCase':
        rel_path = path if base_path == "" else os.path.relpath(path, base_path)
        parts = rel_path.split(os.sep)
        name = os.path.splitext(parts[-1])[0]
        groups = parts[:-1]
        input_file = path

        with open(path, 'r') as f:
            lines = f.readlines()

        # all lines starting with '# expect: <>' are expected outputs
        expected = []
        for line in lines:
            line = line.strip()
            if line.startswith("# expect:"):
                expected.append(line[len("# expect:"):].strip())
        expected_output = "\n".join(expected) if expected else ""

        return TestCase(
            case_name=name,
            groups=groups,
            input_file=input_file,
            expected_output=expected_output
        )

    def full_name(self) -> str:
        return '.'.join(self.groups + [self.case_name])

    def test(self) -> TestResult:
        try:
            result = subprocess.run([executable, self.input_file], capture_output=True, text=True, timeout=5)
            output = result.stdout.strip()
            return_code = result.returncode

            if return_code >= 0:
                if output == self.expected_output or (self.expected_output == COMPERROR and return_code == COMPERROR_CODE):
                    return TestResult(status=TestStatus.PASS)
                else:
                    return TestResult(status=TestStatus.FAIL, details=f"Expected: {self.expected_output}, Got: {output}")
            else:
                error_message = result.stderr.strip() if result.stderr else "No error message."
                error_code_result = os.strerror(return_code)
                return TestResult(status=TestStatus.CRASH, details=f"Process exited with code {return_code} ({error_code_result}). Stderr: {error_message}")
        except Exception as e:
            return TestResult(status=TestStatus.CRASH, details=str(e))

    def __str__(self) -> str:
        return f"TestCase(name={self.case_name}, groups={self.groups}, input_file={self.input_file}, expected_output_file={self.expected_output})"

test_cases: list[TestCase] = []
for root, dirs, files in os.walk(root_dir):
    for file in files:
        if not any(file.endswith(ext) for ext in SLANG_EXTENSIONS): continue

        test_cases.append(TestCase.from_path(os.path.join(root, file), root_dir))

print(f"Found {len(test_cases)} test cases.")

# Sort by full name
test_cases.sort(key=lambda tc: tc.full_name())

# Run tests
print("Running tests...")
counts = {
    TestStatus.PASS: [],
    TestStatus.FAIL: [],
    TestStatus.CRASH: []
}
last_group = ""
for test_case in test_cases:
    new_group = test_case.groups[0] if test_case.groups else test_case.case_name
    if new_group != last_group:
        print(f"\n{new_group}: ", end="")
        last_group = new_group

    # print(f"{test_case.full_name()}: ", end="")
    result = test_case.test()

    if result.status == TestStatus.PASS:
        print("\033[92m.\033[0m", end="")
    elif result.status == TestStatus.FAIL:
        print("\033[93mF\033[0m", end="")
        # print(f"  Details: {result.details}")
    else:
        print("\033[91mC\033[0m", end="")
        # print(f"  Details: {result.details}")

    counts[result.status].append(test_case)

# Print more info on failures and crashes
print('\n')

if counts[TestStatus.FAIL]:
    print("Failures:")
    for test_case in counts[TestStatus.FAIL]:
        result = test_case.test()
        print(f"- {test_case.full_name()}: {result.details}")
    print()

if counts[TestStatus.CRASH]:
    print("Crashes:")
    for test_case in counts[TestStatus.CRASH]:
        result = test_case.test()
        print(f"- {test_case.full_name()}: {result.details}")
    print()

# Summary
print("Test Summary:")
total = sum(len(v) for v in counts.values())
print(f"Total: {total:>2d}")
for status, count in counts.items():
    print(f"{status.value:>5s}: {len(count):>2d} ({(len(count)/total*100) if total > 0 else 0:>5.2f}%)")
