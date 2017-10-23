#!/usr/bin/env ruby
require 'fileutils'

# 新品の設定ファイルを使う。
FileUtils.cp "peercast.ini.master", "peercast.ini"

# 起動。
pid = spawn "peercast-yt/peercast -i peercast.ini"
sleep 0.1

# プロセスは死んだか。
fail 'process is dead' unless Process.wait(-1, Process::WNOHANG) == nil

# プロセスを終了する。
Process.kill(9, pid)

puts 'ok'
