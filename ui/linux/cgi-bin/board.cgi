#!/usr/bin/env python
# -*- coding: utf-8 -*-
import cgi, json, sys

sys.path.append('cgi-bin')

import bbs_reader

form = cgi.FieldStorage()

if "category" not in form or "board_num" not in form:
  common.print_bad_request("bad parameter")
  sys.exit()

board = bbs_reader.Board(form["category"].value, form["board_num"].value)
settings = board.settings()

if "error" in settings:
  data = {
    "status": "error",
    "message": settings["error"]
  }
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

print("Content-Type: text/json; charset=UTF-8\n")
print(json.dumps(data))
