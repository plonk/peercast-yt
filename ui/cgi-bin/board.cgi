#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import cgi, json, sys

sys.path.append('cgi-bin')

import bbs_reader

form = cgi.FieldStorage()

if "fqdn" not in form or "category" not in form:
  common.print_bad_request("bad parameter")
  sys.exit()

board_num = form["board_num"].value if "board_num" in form else ""

board = bbs_reader.Board(form["fqdn"].value, form["category"].value, board_num)
settings = board.settings()

if "error" in settings:
  title = settings["error"]
else:
  title = settings["bbs_title"]

threads = [{
  "id":    thread.id,
  "title": thread.title,
  "last":  thread.last
} for thread in board.threads()]

data = {
  "status":    "ok",
  "title":     title,
  "threads":   threads,
  "category":  board.category,
  "board_num": board.board_num
}

print("Content-Type: application/json; charset=UTF-8\n")
print(json.dumps(data))
