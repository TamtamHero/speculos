import json
import jsonschema
import logging
import os
import pkg_resources
import re

class Automation:
    def __init__(self, document):
        self.logger = logging.getLogger("automation")
        self.variables = {}
        self.current_title = ""
        self.paging_done = False
        self.current_value = ""

        if document.startswith("file:"):
            path = document[5:]
            with open(path) as fp:
                self.json = json.load(fp)
        else:
            self.json = json.loads(document)
        self.validate()

    def validate(self):
        path = os.path.join("resources", "automation.schema")
        with pkg_resources.resource_stream(__name__, path) as fp:
            schema = json.load(fp)
        jsonschema.validate(instance=self.json, schema=schema)

    def set_bool(self, key, value):
        self.variables[key] = value
        
    def parse_input(self, text, y):
        if y == 3:
            m = re.match("(.*) \\((\\d)/(\\d)\\)", text)
            if not m:
                self.paging_done = True
                self.current_title = text
            if m and m.group(2) == m.group(3):
                    self.current_title = m.group(1)
                    self.paging_done = True
        elif y == 17:
            self.current_value += text
            if self.paging_done:
                self.logger.debug(f'{self.current_title}; {self.current_value}')
                self.current_title = ""
                self.current_value = ""
                self.paging_done = False
    
    def get_actions(self, text, x, y):
        text = text.decode("utf-8")
        #self.logger.debug(f'getting actions for "{text}" ({x}, {y})')
        
        self.parse_input(text, y)

        for rule in self.json["rules"]:
            if "text" in rule and rule["text"] != text:
                continue
            if "regexp" in rule and not re.match(rule["regexp"], text):
                continue
            if "x" in rule and rule["x"] != x:
                continue
            if "y" in rule and rule["y"] != y:
                continue
            if "conditions" in rule:
                condition = True
                for key, value in rule["conditions"]:
                    if self.variables.get(key, False) != value:
                        condition = False
                        break
                if not condition:
                    continue

            if not "actions" in rule:
                self.logger.warning(f'missing "actions" key for rule {action}')
                continue

            return rule["actions"]

        return []
