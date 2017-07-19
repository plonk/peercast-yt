#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import cgi, os, subprocess, sys

if __name__ == "__main__":
  form = cgi.FieldStorage()

  for param in ["id"]:
    if param not in form:
      print("Status: 400 Bad Request\n")
      sys.exit()

  id = form["id"].value
  server_name = os.getenv("SERVER_NAME")
  server_port = os.getenv("SERVER_PORT")
  r = int(form["bitrate"].value) # チャンネルのビットレートを映像ビットレートとする。
  if r == 0:
    # 正常なビットレートが渡されなかった場合は 500Kbps にする。
    r = 500

  print("Content-Type: application/vnd.ms-asf\n", flush=True)
  subprocess.call(["ffmpeg",
    "-v", "-8", # quiet
    "-nostdin",
    "-y",       # confirm overwriting
    "-i", "http://{0}:{1}/stream/{2}".format(server_name, server_port, id),
    "-strict", "-2",
    "-b:v",  str(r),
    "-f", "asf",
    "-"])        # to stdout
