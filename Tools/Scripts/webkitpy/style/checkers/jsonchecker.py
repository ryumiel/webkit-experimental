# Copyright (C) 2011 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Checks WebKit style for JSON files."""

import json
import re


class JSONChecker(object):
    """Processes JSON lines for checking style."""

    categories = set(('json/syntax',))

    def __init__(self, file_path, handle_style_error):
        self._handle_style_error = handle_style_error
        self._handle_style_error.turn_off_line_filtering()

    def check(self, lines):
        try:
            json.loads('\n'.join(lines) + '\n')
        except ValueError, e:
            self._handle_style_error(self.line_number_from_json_exception(e), 'json/syntax', 5, str(e))

    @staticmethod
    def line_number_from_json_exception(error):
        match = re.search(r': line (?P<line>\d+) column \d+', str(error))
        if not match:
            return 0
        return int(match.group('line'))


class JSONContributorsChecker(JSONChecker):
    """Processes contributors.json lines"""

    def check(self, lines):
        super(JSONContributorsChecker, self).check(lines)
        self._handle_style_error(0, 'json/syntax', 5, 'contributors.json should not be modified through the commit queue')


class JSONFeaturesChecker(JSONChecker):
    """Processes the features.json lines"""

    def check(self, lines):
        super(JSONFeaturesChecker, self).check(lines)

        try:
            features_definition = json.loads('\n'.join(lines) + '\n')
            if 'features' not in features_definition:
                self._handle_style_error(0, 'json/syntax', 5, '"features" key not found, the key is mandatory.')
                return

            features_list = features_definition['features']
            for i in xrange(len(features_list)):
                feature = features_list[i]
                if 'name' not in feature:
                    self._handle_style_error(0, 'json/syntax', 5, 'The feature %d does not have the mandatory field "name".' % i)
                if 'status' not in feature:
                    self._handle_style_error(0, 'json/syntax', 5, 'The feature %d does not have the mandatory field "status".' % i)
        except:
            pass
