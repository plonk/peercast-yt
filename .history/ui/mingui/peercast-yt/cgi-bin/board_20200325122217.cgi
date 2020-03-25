#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import cgi, json, sys

sys.path.append('cgi-bin')

import bbs_reader

class T:
  def __init__(self, v):
    self.value = v

form = {
  'fqdn' : T("jbbs.shitaraba.net"),
  'category' : T("game"),
  'board_num' : T("49115")
}

#form = cgi.FieldStorage()

if "fqdn" not in form or "category" not in form:
  common.print_bad_request("bad parameter")
  sys.exit()

board_num = form["board_num"].value if form["board_num"].value else ""

board = bbs_reader.Board(form["fqdn"].value, form["category"].value, board_num)
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
