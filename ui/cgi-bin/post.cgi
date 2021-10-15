#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import cgi, json, sys

sys.path.append('cgi-bin')
import bbs_reader

import urllib.request
import urllib.parse

def post_message_nichan(fqdn, category, thread_id, name, mail, body):
  url = f'http://{fqdn}/test/bbs.cgi'
  referer = f'http://{fqdn}/#{category}/'

  form_data = {
    "FROM":name.encode('shift_jis'),
    "mail":mail.encode('shift_jis'),
    "MESSAGE":body.encode('shift_jis'),
    "bbs":category,
    "key":thread_id,
    "submit":"書き込む".encode('shift_jis')
  }
  data = urllib.parse.urlencode(form_data).encode('ascii')
  headers = {'Referer':referer}
  request = urllib.request.Request(url, data, headers)
  response = urllib.request.urlopen(request)

  if response.getcode() == 200:
    return {'status':'ok'}
  else:
    return {'status':'error','code':response.getcode()}

form = cgi.FieldStorage()

# 2ch掲示板の場合は category は板名。name, mail は空文字列でよい。
for key in ['fqdn', 'category', 'id', 'body']:
  if key not in form:
    bbs_reader.print_bad_request(key)
    sys.exit()

name = form['name'].value if 'name' in form else ""
mail = form['mail'].value if 'mail' in form else ""

board_num = form["board_num"].value if "board_num" in form else ""

result = post_message_nichan(form['fqdn'].value,
                             form['category'].value,
                             form['id'].value,
                             name,
                             mail,
                             form['body'].value)

print("Content-Type: application/json; charset=UTF-8\n")
print(json.dumps(result))
