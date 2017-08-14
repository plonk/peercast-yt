#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import cgi, os, subprocess, sys

if __name__ == "__main__":
  form = cgi.FieldStorage()

  for param in ["id", "preset", "audio_codec", "type"]:
    if param not in form:
      print("Status: 400 Bad Request\n")
      sys.exit()

  id = form["id"].value
  preset = form["preset"].value
  audio_codec = form["audio_codec"].value
  server_name = os.getenv("SERVER_NAME")
  server_port = os.getenv("SERVER_PORT")
  r = int(form["bitrate"].value) # チャンネルのビットレートを映像ビットレートとする。
  if r == 0:
    # 正常なビットレートが渡されなかった場合は 500Kbps にする。
    r = 500

  if "type" == "WMV":
    protocol = "mmsh"
  else:
    protocol = "http"

  print("Content-Type: video/x-flv\n", flush=True)
  subprocess.call(["ffmpeg",
    "-nostdin",
    "-v", "-8", # quiet
    "-y",       # confirm overwriting
    "-i", "{0}://{1}:{2}/stream/{3}".format(protocol, server_name, server_port, id),
    "-strict", "-2",
    "-acodec", audio_codec,
    "-async", "1",
    "-ar", "44100",
    "-vcodec", "libx264",
    "-x264-params", "bitrate={0}:vbv-maxrate={0}:vbv-bufsize={1}".format(r, 2*r),
    "-preset", preset,
    "-f", "flv",
    "-"])        # to stdout
