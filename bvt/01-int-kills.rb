#!/usr/bin/env ruby
require 'fileutils'

# 新品の設定ファイルを使う。
FileUtils.cp "peercast.ini.master", "peercast.ini"

# 起動。
pid = spawn "peercast-yt/peercast -i peercast.ini"
sleep 0.1

# プロセスを終了する。
Process.kill(:INT, pid)

t0 = Time.now
loop do
  # 10 秒超待ってもプロセスが死ななかったら失敗。
  if Time.now - t0 > 10
    fail
  end
  pid, status = Process.wait2(-1, Process::WNOHANG)
  if pid && status.exited?
    break
  end
  sleep 0.1
end

puts 'ok'
