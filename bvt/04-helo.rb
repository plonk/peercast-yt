require_relative 'common'
require 'socket'

require_relative 'atom'
include Atom

def get_version(tip)
  ip, port = tip.split(':')

  s = TCPSocket.new(ip, port.to_i)
  s.write Atom::Integer.new(PCP_CONNECT, 1).to_s

  helo = Atom::Parent.new(PCP_HELO)
  helo << Atom::String.new(PCP_HELO_AGENT, PCX_AGENT)
  helo << Atom::Integer.new(PCP_HELO_VERSION, 1218)
  helo << Atom::Bytes.new(PCP_HELO_SESSIONID, "1234567890123456")
  helo << Atom::Short.new(PCP_HELO_PORT, 8144)

  # HELO
  s.write helo
  # OLEH
  atom = Atom::read(s)
  atom['agnt'] && atom['agnt'].value
end

spawn_peercast

ver = get_version('127.0.0.1:7144')

if ver =~ /\APeerCast\/0.1218 \(YT\d\d\)\z/
  puts 'ok'
else
  STDERR.puts ver.inspect
  exit 1
end
