#!/usr/bin/env ruby -W0
require 'fileutils'
require_relative 'common'

if RUBY_PLATFORM =~ /msys/
  puts "skipped"
  exit 0
end

# 新品の設定ファイルを使う。
FileUtils.cp "peercast.ini.master", "peercast.ini"

# 起動。
pid = spawn_peercast

# プロセスを終了する。
Process.kill(:INT, pid)

t0 = Time.now
loop do
  # 10 秒超待ってもプロセスが死ななかったら失敗。
  if Time.now - t0 > 10
    fail
  end
  pid, status = Process.wait2(-1, Process::WNOHANG)
  if pid
    if status.exited?
      break
    else
      # SIGINT シグナルハンドラーのインストールが間に合わないとここに来る。
      puts "process died abnormally: #{status.inspect}"
      exit 1
    end
  end
  sleep 0.1
end

puts 'ok'
