require 'fileutils'

def spawn_peercast
  # 新品の設定ファイルを使う。
  FileUtils.cp "peercast.ini.master", "peercast.ini"

  # 起動。
  pid = spawn "peercast-yt/peercast -i peercast.ini"
  sleep 0.1

  at_exit {
    Process.kill(9, pid)
  }

  fail "peercast dead immediately after spawn" if Process.wait(-1, Process::WNOHANG) != nil
end

def assert_eq(expectation, actual, opts = {})
  context = opts[:context] ? "(#{ opts[:context]})" : ""
  if expectation != actual
    fail "#{expectation.inspect} expected but got #{actual.inspect}#{context}"
  end
end

def assert(actual, opts = {})
  context = opts[:context] ? "(#{ opts[:context]})" : ""
  unless actual
    fail "#{actual.inspect} is not true-ish#{context}"
  end
end
