#!/usr/bin/env python3
"""
Generate API_COVERAGE.md from lcov coverage_report.txt
"""

import sys
import re

def parse_coverage_report(input_file):
    """Parse the lcov coverage report text file."""
    with open(input_file, 'r') as f:
        lines = f.readlines()

    files = []
    summary = {}

    for line in lines:
        line = line.strip()

        # Parse summary line
        if 'Total:' in line:
            parts = line.split('|')
            if len(parts) >= 3:
                # Extract total line coverage
                line_match = re.search(r'(\d+\.?\d*)%\s+(\d+)', parts[1])
                if line_match:
                    summary['lines_rate'] = line_match.group(1)
                    summary['lines_num'] = line_match.group(2)

                # Extract total function coverage
                func_match = re.search(r'(\d+\.?\d*)%\s+(\d+)', parts[2])
                if func_match:
                    summary['functions_rate'] = func_match.group(1)
                    summary['functions_num'] = func_match.group(2)

        # Parse file lines (skip header and separator lines)
        elif '|' in line and not line.startswith('Filename') and not line.startswith('===') and not line.startswith('['):
            parts = [p.strip() for p in line.split('|')]
            if len(parts) >= 3:
                filename = parts[0]

                # Extract line coverage
                line_match = re.search(r'(\d+\.?\d*)%\s+(\d+)', parts[1])
                # Extract function coverage
                func_match = re.search(r'(\d+\.?\d*)%\s+(\d+)', parts[2])

                if filename and line_match and func_match:
                    files.append({
                        'filename': filename,
                        'lines_rate': line_match.group(1),
                        'lines_num': line_match.group(2),
                        'functions_rate': func_match.group(1),
                        'functions_num': func_match.group(2)
                    })

    return summary, files

def generate_markdown(summary, files, output_file):
    """Generate markdown coverage report."""
    with open(output_file, 'w') as f:
        # Header
        f.write("# API Coverage Report\n\n")

        # Summary table
        f.write("## Summary\n\n")
        f.write("| Metric | Rate | Total |\n")
        f.write("|--------|------|-------|\n")
        f.write(f"| **Lines** | {summary.get('lines_rate', 'N/A')}% | {summary.get('lines_num', 'N/A')} |\n")
        f.write(f"| **Functions** | {summary.get('functions_rate', 'N/A')}% | {summary.get('functions_num', 'N/A')} |\n")
        f.write("\n---\n\n")

        # Detailed coverage table
        f.write("## Detailed Coverage by File\n\n")
        f.write("| Filename | Lines Rate | Lines Num | Functions Rate | Functions Num |\n")
        f.write("|----------|------------|-----------|----------------|---------------|\n")

        for file_info in files:
            f.write(f"| **{file_info['filename']}** | "
                   f"{file_info['lines_rate']}% | "
                   f"{file_info['lines_num']} | "
                   f"{file_info['functions_rate']}% | "
                   f"{file_info['functions_num']} |\n")

        f.write("\n---\n\n")
        f.write("*Generated from lcov coverage analysis*\n")

def main():
    if len(sys.argv) != 3:
        print("Usage: generate_coverage_md.py <input_coverage_report.txt> <output_API_COVERAGE.md>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    try:
        summary, files = parse_coverage_report(input_file)
        generate_markdown(summary, files, output_file)
        print(f"Successfully generated {output_file}")
    except Exception as e:
        print(f"Error generating coverage markdown: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
