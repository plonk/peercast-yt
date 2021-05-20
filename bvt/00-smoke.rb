#!/usr/bin/env ruby -W0
require 'fileutils'
require_relative 'common'

# 新品の設定ファイルを使う。
FileUtils.cp "peercast.ini.master", "peercast.ini"

# 起動。
pid = spawn_peercast

# プロセスは死んだか。
fail 'process is dead' unless Process.wait(-1, Process::WNOHANG) == nil

# プロセスを終了する。
Process.kill(9, pid)

puts 'ok'
