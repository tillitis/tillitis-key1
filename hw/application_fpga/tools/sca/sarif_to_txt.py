#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: BSD-2-Clause

import argparse
import json
import sys
import re
from pathlib import Path
from typing import Dict, List, Optional, TextIO
from contextlib import nullcontext

# ============================================================
# Utility
# ============================================================

def safe_get(dictionary, *keys, default=None):
    current = dictionary
    for key in keys:
        try:
            current = current[key]
        except (KeyError, IndexError, TypeError):
            return default
    return current

def read_file_lines_cached(path: str, cache: Dict[str, List[str]]) -> List[str]:

    if path not in cache:
        try:
            with open(path, "r") as f:
                cache[path] = f.readlines()
        except OSError:
            print("Could not open filename: " + path)
            sys.exit(1)

    return cache[path]

# ============================================================
# Censor Rules
# ============================================================

class CensorRule:

    def __init__(self, raw: str):

        parts = raw.split(":")

        if len(parts) != 4:
            print("Malformed --censor-report string:")
            for item in parts:
                print(item + ":", end="")
            print("")
            self.valid = False
            return

        self.issue_type = parts[0]
        self.filename = parts[1]
        self.function = parts[2]
        self.reason = parts[3]

        self.compare = f"{self.issue_type}:{self.filename}:{self.function}"
        self.valid = True

    def matches(self, compare_string: str) -> bool:
        return bool(re.search(self.compare, compare_string))

# ============================================================
# Issue Extraction
# ============================================================

def extract_issue_data(result: dict):

    start_line = safe_get(
        result,
        "locations", 0,
        "physicalLocation", "region", "startLine",
        default="-"
    )

    end_line = safe_get(
        result,
        "locations", 0,
        "physicalLocation", "region", "endLine"
    )

    if end_line is None:
        end_line = start_line

    function = safe_get(
        result,
        "locations", 0,
        "logicalLocations", "fullyQualifiedName",
        default="-"
    )

    if not function:
        function = "-"

    return {
        "file": safe_get(result, "locations", 0, "physicalLocation", "artifactLocation", "uri", default="-"),
        "start_line": start_line,
        "end_line": end_line,
        "function": function,
        "type": safe_get(result, "ruleId", default="-"),
        "message": safe_get(result, "message", "text", default="-"),
    }

# ============================================================
# Reporting
# ============================================================

def write_basic_report(out: TextIO, issue_index: int, issue, context_lines):

    out.write(
        f"#{issue_index}\n"
        f"{issue['file']}:{issue['start_line']}::{issue['function']}() "
        f"Error: {issue['type']}\n"
        f"  {issue['message']}\n"
    )

    line_number = issue["start_line"] - 2

    for line in context_lines:
        out.write(f"  {line_number}. {line.rstrip()}\n")
        line_number += 1

    out.write("\n")

def write_expanded_header(out: TextIO, issue_index: int, issue):

    out.write(
        f"#{issue_index}\n"
        f"{issue['file']}:{issue['start_line']}::{issue['function']}() "
        f"Error: {issue['type']}\n"
        f"  {issue['message']}\n\n"
    )

# ============================================================
# SARIF Processing
# ============================================================

def process_sarif_file(
    sarif_file: Path,
    args,
    censor_rules,
    basic_out,
    expanded_out,
    censored_out,
    issue_counter
):

    if args.verbose:
        print("Parsing file: " + str(sarif_file))

    try:
        with open(sarif_file, "r") as f:
            sarif = json.load(f)
    except Exception:
        print("Failed to parse sarif file: " + str(sarif_file))
        sys.exit(1)

    try:
        results = sarif["runs"][0]["results"]
    except Exception:
        print('Could not find json key "runs.results". in file: ' + str(sarif_file))
        sys.exit(1)

    file_cache: Dict[str, List[str]] = {}

    for result in results:

        issue = extract_issue_data(result)

        compare_string = f"{issue['type']}:{issue['file']}:{issue['function']}"

        print_issue = True

        for rule in censor_rules:

            if not rule.valid:
                continue

            if rule.matches(compare_string):

                print_issue = False

                if censored_out:
                    censored_out.write(
                        f"\"{issue['type']}:{issue['file']}:{issue['end_line']}:{issue['function']}:{rule.reason}\"\n"
                    )

        if print_issue:

            lines = read_file_lines_cached(issue["file"], file_cache)

            start = issue["start_line"] - 3
            end = issue["end_line"] + 2

            context = lines[start:end]

            if basic_out:
                write_basic_report(basic_out, issue_counter, issue, context)

            if expanded_out:
                write_expanded_header(expanded_out, issue_counter, issue)

            indent_count = 0

            try:
                flow_locations = result["codeFlows"][0]["threadFlows"][0]["locations"]
            except Exception:
                print('Could not find json key "runs.results.codeFlows.locations". in file: ' + str(sarif_file))
                sys.exit(1)

            for loc in flow_locations:

                file_name = safe_get(loc, "location", "physicalLocation", "artifactLocation", "uri", default="-")

                start_line = safe_get(loc, "location", "physicalLocation", "region", "startLine", default="-")

                end_line = safe_get(loc, "location", "physicalLocation", "region", "endLine")

                if end_line is None:
                    end_line = start_line

                column = safe_get(loc, "location", "physicalLocation", "region", "startColumn", default="-")

                message = safe_get(loc, "location", "message", "text", default="-")

                lines = read_file_lines_cached(file_name, file_cache)

                start = start_line - 3
                end = end_line + 2

                context = lines[start:end]

                if expanded_out:
                    expanded_out.write(
                        f"{file_name}:{start_line}:{column}: {message}\n"
                    )

                line_number = start_line - 2

                for line in context:

                    if expanded_out:
                        expanded_out.write(
                            "  "
                            + indent_count * " "
                            + f"{line_number}. {line.rstrip()}\n"
                        )

                    line_number += 1

                if re.search("Calling", message):
                    indent_count += 2

                if expanded_out:
                    expanded_out.write("\n")

        issue_counter += 1

    return issue_counter

# ============================================================
# CLI
# ============================================================

def parse_args():

    parser = argparse.ArgumentParser()

    parser.add_argument("--basic-report")
    parser.add_argument("--expanded-report")
    parser.add_argument("--censored-issues-report")
    parser.add_argument("--censor-report", action="append")
    parser.add_argument("-v", "--verbose", action="store_true")

    parser.add_argument("sarif_files", nargs="+")

    return parser.parse_args()

# ============================================================
# Main
# ============================================================

def main():

    args = parse_args()

    censor_rules = []

    if args.censor_report:
        for r in args.censor_report:
            censor_rules.append(CensorRule(r))

    if args.verbose and args.basic_report:
        print("Basic report file: " + args.basic_report)

    if args.verbose and args.expanded_report:
        print("Expanded report file: " + args.expanded_report)

    if args.verbose and args.censored_issues_report:
        print("Censored issues report file: " + args.censored_issues_report)

    with (
        open(args.basic_report, "w") if args.basic_report else nullcontext() as basic_out,
        open(args.expanded_report, "w") if args.expanded_report else nullcontext() as expanded_out,
        open(args.censored_issues_report, "w") if args.censored_issues_report else nullcontext() as censored_out,
    ):

        issue_counter = 0

        for sarif_file in args.sarif_files:

            issue_counter = process_sarif_file(
                Path(sarif_file),
                args,
                censor_rules,
                basic_out,
                expanded_out,
                censored_out,
                issue_counter
            )


if __name__ == "__main__":
    main()
