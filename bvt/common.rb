require 'fileutils'

def spawn_peercast
  # 新品の設定ファイルを使う。
  FileUtils.cp "peercast.ini.master", "peercast-yt/peercast.ini"

  # 起動。
  if RUBY_PLATFORM =~ /msys/
    cmdline = "./peercast.exe"
  else
    cmdline = "./peercast -i peercast.ini -P ."
  end
  Dir.chdir("peercast-yt")
  pid = spawn cmdline
  Dir.chdir("..")
  sleep 1.0

  at_exit {
    Process.kill(9, pid) rescue nil
  }

  fail "peercast died immediately after spawn" if Process.wait(-1, Process::WNOHANG) != nil

  return pid
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

require 'net/http'
require 'ostruct'

def http_get(url)
  uri = URI(url)
  Net::HTTP.start(uri.host.gsub(/[\[\]]/,''), uri.port) do |http|
    path = if uri.path.empty? then "/" else uri.path end
    res = http.request_get(path)
    return OpenStruct.new(code: res.code.to_i,
                          headers: res,
                          body: res.body)
  end
end

def http_post(url, opts)
  data = opts[:body] || ""
  
  uri = URI(url)
  Net::HTTP.start(uri.host.gsub(/[\[\]]/,''), uri.port) do |http|
    path = if uri.path.empty? then "/" else uri.path end
    res = http.request_post(path, data)
    return OpenStruct.new(code: res.code.to_i,
                          headers: res,
                          body: res.body)
  end
end
