#!/usr/bin/env python
# Copyright (C) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import logging
import os

from webkitpy.layout_tests.layout_package import json_results_generator
from webkitpy.layout_tests.layout_package import test_expectations
from webkitpy.layout_tests.layout_package import test_failures
import webkitpy.thirdparty.simplejson as simplejson

class JSONLayoutResultsGenerator(json_results_generator.JSONResultsGenerator):
    """A JSON results generator for layout tests."""

    LAYOUT_TESTS_PATH = "LayoutTests"

    # Additional JSON fields.
    WONTFIX = "wontfixCounts"
    DEFERRED = "deferredCounts"

    def __init__(self, port, builder_name, build_name, build_number,
        results_file_base_path, builder_base_url,
        test_timings, expectations, result_summary, all_tests):
        """Modifies the results.json file. Grabs it off the archive directory
        if it is not found locally.

        Args:
          result_summary: ResultsSummary object storing the summary of the test
              results.
          (see the comment of JSONResultsGenerator.__init__ for other Args)
        """
        self._port = port
        self._builder_name = builder_name
        self._build_name = build_name
        self._build_number = build_number
        self._builder_base_url = builder_base_url
        self._results_file_path = os.path.join(results_file_base_path,
            self.RESULTS_FILENAME)
        self._expectations = expectations

        # We don't use self._skipped_tests and self._passed_tests as we
        # override _InsertFailureSummaries.

        # We want relative paths to LayoutTest root for JSON output.
        path_to_name = self._get_path_relative_to_layout_test_root
        self._result_summary = result_summary
        self._failures = dict(
            (path_to_name(test), test_failures.determine_result_type(failures))
            for (test, failures) in result_summary.failures.iteritems())
        self._all_tests = [path_to_name(test) for test in all_tests]
        self._test_timings = dict(
            (path_to_name(test_tuple.filename), test_tuple.test_run_time)
            for test_tuple in test_timings)
        self._svn_repositories = port.test_repository_paths()

        self._generate_json_output()

    def _get_path_relative_to_layout_test_root(self, test):
        """Returns the path of the test relative to the layout test root.
        For example, for:
          src/third_party/WebKit/LayoutTests/fast/forms/foo.html
        We would return
          fast/forms/foo.html
        """
        index = test.find(self.LAYOUT_TESTS_PATH)
        if index is not -1:
            index += len(self.LAYOUT_TESTS_PATH)

        if index is -1:
            # Already a relative path.
            relativePath = test
        else:
            relativePath = test[index + 1:]

        # Make sure all paths are unix-style.
        return relativePath.replace('\\', '/')

    # override
    def _convert_json_to_current_version(self, results_json):
        archive_version = None
        if self.VERSION_KEY in results_json:
            archive_version = results_json[self.VERSION_KEY]

        super(JSONLayoutResultsGenerator,
              self)._convert_json_to_current_version(results_json)

        # version 2->3
        if archive_version == 2:
            for results_for_builder in results_json.itervalues():
                try:
                    test_results = results_for_builder[self.TESTS]
                except:
                    continue

            for test in test_results:
                # Make sure all paths are relative
                test_path = self._get_path_relative_to_layout_test_root(test)
                if test_path != test:
                    test_results[test_path] = test_results[test]
                    del test_results[test]

    # override
    def _insert_failure_summaries(self, results_for_builder):
        summary = self._result_summary

        self._insert_item_into_raw_list(results_for_builder,
            len((set(summary.failures.keys()) |
                summary.tests_by_expectation[test_expectations.SKIP]) &
                summary.tests_by_timeline[test_expectations.NOW]),
            self.FIXABLE_COUNT)
        self._insert_item_into_raw_list(results_for_builder,
            self._get_failure_summary_entry(test_expectations.NOW),
            self.FIXABLE)
        self._insert_item_into_raw_list(results_for_builder,
            len(self._expectations.get_tests_with_timeline(
                test_expectations.NOW)), self.ALL_FIXABLE_COUNT)
        self._insert_item_into_raw_list(results_for_builder,
            self._get_failure_summary_entry(test_expectations.DEFER),
            self.DEFERRED)
        self._insert_item_into_raw_list(results_for_builder,
            self._get_failure_summary_entry(test_expectations.WONTFIX),
            self.WONTFIX)

    # override
    def _normalize_results_json(self, test, test_name, tests):
        super(JSONLayoutResultsGenerator, self)._normalize_results_json(
            test, test_name, tests)

        # Remove tests that don't exist anymore.
        full_path = os.path.join(self._port.layout_tests_dir(), test_name)
        full_path = os.path.normpath(full_path)
        if not os.path.exists(full_path):
            del tests[test_name]

    def _get_failure_summary_entry(self, timeline):
        """Creates a summary object to insert into the JSON.

        Args:
          summary   ResultSummary object with test results
          timeline  current test_expectations timeline to build entry for
                    (e.g., test_expectations.NOW, etc.)
        """
        entry = {}
        summary = self._result_summary
        timeline_tests = summary.tests_by_timeline[timeline]
        entry[self.SKIP_RESULT] = len(
            summary.tests_by_expectation[test_expectations.SKIP] &
            timeline_tests)
        entry[self.PASS_RESULT] = len(
            summary.tests_by_expectation[test_expectations.PASS] &
            timeline_tests)
        for failure_type in summary.tests_by_expectation.keys():
            if failure_type not in self.FAILURE_TO_CHAR:
                continue
            count = len(summary.tests_by_expectation[failure_type] &
                        timeline_tests)
            entry[self.FAILURE_TO_CHAR[failure_type]] = count
        return entry
