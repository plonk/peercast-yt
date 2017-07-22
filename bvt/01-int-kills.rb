#!/usr/bin/env ruby
require 'fileutils'

# 新品の設定ファイルを使う。
FileUtils.cp "peercast.ini.master", "peercast.ini"

# 起動。
pid = spawn "peercast-yt/peercast -i peercast.ini"
sleep 0.1

# プロセスを終了する。
Process.kill(:INT, pid)

# プロセスは死んだか。
if Process.wait(-1, Process::WNOHANG) != nil
  fail
end

puts 'ok'
