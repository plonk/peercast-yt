require_relative 'common'
require 'socket'

require_relative 'atom'
include Atom

def helo(tip)
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
  return Atom::read(s)
end

spawn_peercast

oleh = helo('127.0.0.1:7144')

fail 'oleh' unless oleh.id4 == 'oleh'
fail 'agnt' unless oleh['agnt'] && oleh['agnt'].value =~ /\APeerCast\/0.1218 \(YT\d\d\)\z/
fail 'sid' unless oleh['sid']
fail 'ver' unless oleh['ver'] && oleh['ver'].value == 1218
fail 'rip' unless oleh['rip']
fail 'port' unless oleh['port']

puts 'ok'
