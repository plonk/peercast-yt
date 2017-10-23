module Atom
  class String; end
  class Integer; end
  class Short; end
  class Char; end
  class Bytes; end

  ATOM_TABLE = [
    ['PCP_CONNECT', "pcp\n", nil],
    ['PCP_OK', "ok", Atom::Integer],
    ['PCP_HELO', "helo", nil],
    ['PCP_HELO_AGENT', "agnt", Atom::String],
    ['PCP_HELO_OSTYPE', "ostp", Atom::Integer],
    ['PCP_HELO_SESSIONID', "sid", Atom::Bytes],
    ['PCP_HELO_PORT', "port", Atom::Short],
    ['PCP_HELO_PING', "ping", Atom::Short],
    ['PCP_HELO_PONG', "pong", nil],
    ['PCP_HELO_REMOTEIP', "rip", Atom::Integer],
    ['PCP_HELO_VERSION', "ver", Atom::Integer],
    ['PCP_HELO_BCID', "bcid", nil],
    ['PCP_HELO_DISABLE', "dis", nil],
    ['PCP_OLEH', "oleh", nil],
    ['PCP_MODE', "mode", nil],
    ['PCP_MODE_GNUT06', "gn06", nil],
    ['PCP_ROOT', "root", nil],
    ['PCP_ROOT_UPDINT', "uint", nil],
    ['PCP_ROOT_CHECKVER', "chkv", nil],
    ['PCP_ROOT_URL', "url", nil],
    ['PCP_ROOT_UPDATE', "upd", nil],
    ['PCP_ROOT_NEXT', "next", nil],
    ['PCP_OS_LINUX', "lnux", nil],
    ['PCP_OS_WINDOWS', "w32", nil],
    ['PCP_OS_OSX', "osx", nil],
    ['PCP_OS_WINAMP', "wamp", nil],
    ['PCP_OS_ZAURUS', "zaur", nil],
    ['PCP_GET', "get", nil],
    ['PCP_GET_ID', "id", Atom::Bytes],
    ['PCP_GET_NAME', "name", nil],
    ['PCP_HOST', "host", nil],
    ['PCP_HOST_ID', "id", nil],
    ['PCP_HOST_IP', "ip", Atom::Integer],
    ['PCP_HOST_PORT', "port", nil],
    ['PCP_HOST_NUML', "numl", Atom::Integer],
    ['PCP_HOST_NUMR', "numr", Atom::Integer],
    ['PCP_HOST_UPTIME', "uptm", Atom::Integer],
    ['PCP_HOST_TRACKER', "trkr", nil],
    ['PCP_HOST_CHANID', "cid", Atom::Bytes],
    ['PCP_HOST_VERSION', "ver", nil],
    ['PCP_HOST_VERSION_VP', "vevp", Atom::Integer],
    ['PCP_HOST_VERSION_EX_PREFIX', "vexp", Atom::Bytes],
    ['PCP_HOST_VERSION_EX_NUMBER', "vexn", Atom::Short],
    ['PCP_HOST_CLAP_PP', "clap", nil],
    ['PCP_HOST_FLAGS1', "flg1", Atom::Char],
    ['PCP_HOST_OLDPOS', "oldp", Atom::Integer],
    ['PCP_HOST_NEWPOS', "newp", Atom::Integer],
    ['PCP_HOST_UPHOST_IP', "upip", Atom::Integer],
    ['PCP_HOST_UPHOST_PORT', "uppt", Atom::Integer],
    ['PCP_HOST_UPHOST_HOPS', "uphp", Atom::Integer],
    ['PCP_QUIT', "quit", Atom::Integer],
    ['PCP_CHAN', "chan", nil],
    ['PCP_CHAN_ID', "id", Atom::Bytes],
    ['PCP_CHAN_BCID', "bcid", nil],
    ['PCP_CHAN_KEY', "key", nil],
    ['PCP_CHAN_PKT', "pkt", nil],
    ['PCP_CHAN_PKT_TYPE', "type", nil],
    ['PCP_CHAN_PKT_POS', "pos", Atom::Integer],
    ['PCP_CHAN_PKT_HEAD', "head", nil],
    ['PCP_CHAN_PKT_DATA', "data", nil],
    ['PCP_CHAN_PKT_META', "meta", nil],
    ['PCP_CHAN_PKT_CONTINUATION', "cont", nil],
    ['PCP_CHAN_INFO', "info", nil],
    ['PCP_CHAN_INFO_TYPE', "type", nil],
    ['PCP_CHAN_INFO_BITRATE', "bitr", nil],
    ['PCP_CHAN_INFO_GENRE', "gnre", nil],
    ['PCP_CHAN_INFO_NAME', "name", nil],
    ['PCP_CHAN_INFO_URL', "url", nil],
    ['PCP_CHAN_INFO_DESC', "desc", nil],
    ['PCP_CHAN_INFO_COMMENT', "cmnt", nil],
    ['PCP_CHAN_INFO_PPFLAGS', "pflg", nil],
    ['PCP_CHAN_TRACK', "trck", nil],
    ['PCP_CHAN_TRACK_TITLE', "titl", nil],
    ['PCP_CHAN_TRACK_CREATOR', "crea", nil],
    ['PCP_CHAN_TRACK_URL', "url", nil],
    ['PCP_CHAN_TRACK_ALBUM', "albm", nil],
    ['PCP_MESG', "mesg", nil],
    ['PCP_MESG_ASCII', "asci", nil],
    ['PCP_MESG_SJIS', "sjis", nil],
    ['PCP_BCST', "bcst" , nil],
    ['PCP_BCST_TTL', "ttl" , nil],
    ['PCP_BCST_HOPS', "hops" , nil],
    ['PCP_BCST_FROM', "from" , nil],
    ['PCP_BCST_DEST', "dest" , nil],
    ['PCP_BCST_GROUP', "grp" , nil],
    ['PCP_BCST_CHANID', "cid" , nil],
    ['PCP_BCST_VERSION', "vers", nil],
    ['PCP_BCST_VERSION_VP', "vrvp", nil],
    ['PCP_BCST_VERSION_EX_PREFIX', "vexp", nil],
    ['PCP_BCST_VERSION_EX_NUMBER', "vexn", nil],
    ['PCP_PUSH', "push" , nil],
    ['PCP_PUSH_IP', "ip" , Atom::Integer],
    ['PCP_PUSH_PORT', "port" , nil],
    ['PCP_PUSH_CHANID', "cid" , nil],
    ['PCP_SPKT', "spkt", nil],
    ['PCP_ATOM', "atom" , nil],
    ['PCP_SESSIONID', "sid", Atom::Bytes],
    ['PCP_CHAN_INFO_STREAMTYPE', "styp", Atom::String],
    ['PCP_CHAN_INFO_STREAMEXT', "sext", Atom::String],
  ]

  ATOM_TABLE.each do |name, value, _kind|
    Object.const_set(name, value)
  end

  # --------------------------------------------------------

  PCP_BCST_GROUP_ALL         = 0xff
  PCP_BCST_GROUP_ROOT        = 1
  PCP_BCST_GROUP_TRACKERS    = 2
  PCP_BCST_GROUP_RELAYS      = 4


  PCP_ERROR_QUIT             = 1000
  PCP_ERROR_BCST             = 2000
  PCP_ERROR_READ             = 3000
  PCP_ERROR_WRITE            = 4000
  PCP_ERROR_GENERAL          = 5000

  PCP_ERROR_SKIP             = 1
  PCP_ERROR_ALREADYCONNECTED = 2
  PCP_ERROR_UNAVAILABLE      = 3
  PCP_ERROR_LOOPBACK         = 4
  PCP_ERROR_NOTIDENTIFIED    = 5
  PCP_ERROR_BADRESPONSE      = 6
  PCP_ERROR_BADAGENT         = 7
  PCP_ERROR_OFFAIR           = 8
  PCP_ERROR_SHUTDOWN         = 9
  PCP_ERROR_NOROOT           = 10
  PCP_ERROR_BANNED           = 11

  PCP_HOST_FLAGS1_TRACKER    = 0x01
  PCP_HOST_FLAGS1_RELAY      = 0x02
  PCP_HOST_FLAGS1_DIRECT     = 0x04
  PCP_HOST_FLAGS1_PUSH       = 0x08
  PCP_HOST_FLAGS1_RECV       = 0x10
  PCP_HOST_FLAGS1_CIN        = 0x20
  PCP_HOST_FLAGS1_PRIVATE    = 0x40

  PCX_AGENT = "PeerCast/0.1218"

  class Parent
    attr_reader :id4, :children

    def initialize(id4, *children)
      @id4 = id4.sub(/\0+\z/, '')
      @children = children
    end

    def <<(data_atom)
      @children << data_atom
    end

    def [](id4)
      id4_ = id4.sub(/\0+\z/, '')
      children.find { |child| child.id4 == id4_ }
    end

    def to_s
      [@id4].pack('a4') +
        [@children.size | 0x80000000].pack('V') +
        @children.join
    end

    def inspect
      "#<Atom::Parent #{@id4.sub(/\0+$/, '').inspect} #{@children.inspect}>"
    end
  end

  module DataAtom
    # インクルードするクラスは id4 と bytes のアクセッサーを提供する。

    def to_s
      [id4].pack('a4') +
        [bytes.bytesize].pack('V') +
        bytes
    end

    # デフォルト実装。
    def value
      bytes
    end
  end

  class String
    include DataAtom
    attr :id4, :bytes

    def initialize(id4, s)
      @id4 = id4.sub(/\0+\z/, '')
      len = s.index("\0") || s.bytesize
      @bytes = [s[0,len]].pack("a#{len + 1}")
    end

    def inspect
      "#<Atom::String #{@id4.sub(/\0+$/, '').inspect} #{@bytes.sub(/\0$/, '').inspect}>"
    end

    def value
      bytes.sub(/\0+\z/, '')
    end

    class << self
      def read(id4, len, input)
        data = input.read(len).unpack("a#{len}")[0]
        return String.new(id4, data)
      end
    end
  end

  class Integer
    include DataAtom
    attr :id4, :bytes

    def initialize(id4, n)
      # 32-bit unsigned little-endian
      @id4 = id4.sub(/\0+\z/, '')
      @bytes = [n].pack('V')
    end

    def inspect
      "#<Atom::Integer #{id4.sub(/\0+$/, '').inspect} #{value}>"
    end

    def value
      @bytes.unpack('V')[0]
    end

    class << self
      def read(id4, len, input)
        fail unless len == 4

        data = input.read(4).unpack('V')[0]
        return Integer.new(id4, data)
      end
    end
  end

  class Short
    include DataAtom
    attr :id4, :bytes

    def initialize(id4, n)
      # 16-bit unsigned little-endian
      @id4 = id4.sub(/\0+\z/, '')
      @bytes = [n].pack('v')
    end

    def value
      @bytes.unpack('v')[0]
    end

    class << self
      def read(id4, len, input)
        fail unless len == 2

        data = input.read(2).unpack('v')[0]
        return Short.new(id4, data)
      end
    end
  end

  class Char
    include DataAtom
    attr :id4, :bytes

    def initialize(id4, n)
      # 8-bit unsigned
      @id4 = id4.sub(/\0+\z/, '')
      @bytes = [n].pack('C')
    end

    def value
      @bytes.unpack('C')[0]
    end

    class << self
      def read(id4, len, input)
        fail unless len == 1

        data = input.read(1).unpack('C')[0]
        return Char.new(id4, data)
      end
    end
  end

  class Bytes
    include DataAtom
    attr :id4, :bytes

    def initialize(id4, str)
      @id4 = id4.sub(/\0+\z/, '')
      @bytes = [str].pack("a#{str.bytesize}")
    end

    def inspect
      dump = bytes.bytes.map { |c| '%02X' % c }.join(' ')
      "#<Atom::Bytes #{id4.sub(/\0+/, '').inspect} #{dump}>"
    end

    class << self
      def read(id4, len, input)
        data = input.read(len)
        return Bytes.new(id4, data)
      end
    end
  end

  # IO → atom
  def read(input)
    id4 = input.read(4)
    num = input.read(4).unpack('V')[0]

    if num & 0x80000000 != 0
      # ペアレントアトムである。
      parent = Parent.new(id4)
      num_children = num & 0x7fffffff
      num_children.times do
        atom = read(input)
        parent << atom
      end
      return parent
    else
      # データアトムである。
      id4_ = id4.sub(/\0+\z/, '')
      row = ATOM_TABLE.find { |_name, value, _kind|
        value == id4_
      }
      if row
        kind = row[2]
      else
        STDERR.print "Warning: no entry for #{id4_.inspect}\n"
        kind = Atom::Bytes
      end

      if kind.nil?
        # STDERR.print "Warning: type unknown for #{id4.inspect} defaulting to Bytes\n"
        kind = Atom::Bytes
      end

      atom = kind.read(id4, num, input)
      return atom
    end
  end
  module_function :read

end
