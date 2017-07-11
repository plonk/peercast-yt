#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import cgi, os, subprocess, sys

if __name__ == "__main__":
  form = cgi.FieldStorage()

  for param in ["id", "preset", "audio_codec"]:
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

  print("Content-Type: video/x-flv\n", flush=True)
<<<<<<< HEAD
  ffmpeg = subprocess.Popen(["ffmpeg",
         "-v", "-8", # quiet
         "-y",       # confirm overwriting
         "-i", "mmsh://{0}:{1}/stream/{2}.wmv".format(server_name, server_port, id),
         "-acodec", "aac",
         "-vcodec", "libx264",
         "-x264-params", "bitrate={0}:vbv-maxrate={0}:vbv-bufsize={1}".format(r, 2*r),
         "-preset", "ultrafast",
         "-f", "flv",
         "-"], stdout=subprocess.PIPE)        # to stdout

  for line in ffmpeg.stdout:
    sys.stdout.buffer.write(line)
=======
  subprocess.call(["ffmpeg",
    "-v", "-8", # quiet
    "-y",       # confirm overwriting
    "-i", "mmsh://{0}:{1}/stream/{2}.wmv".format(server_name, server_port, id),
    "-strict", "-2",
    "-acodec", audio_codec,
    "-ar", "44100",
    "-vcodec", "libx264",
    "-x264-params", "bitrate={0}:vbv-maxrate={0}:vbv-bufsize={1}".format(r, 2*r),
    "-preset", preset,
    "-f", "flv",
    "-"])        # to stdout
>>>>>>> plonk/master
