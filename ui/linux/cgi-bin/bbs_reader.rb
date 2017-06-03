require 'net/http'
require 'uri'

# Ruby 2.0 の為の Enumerable#to_h ポリフィル。
unless Enumerable.instance_methods(false).include?(:to_h)
  module Enumerable
    def to_h
      Hash[*to_a.flatten]
    end
  end
end

module Bbs

class HTTPError < StandardError
  def code
    message.to_i
  end
end

SHITARABA_THREAD_URL_PATTERN = %r{\Ahttp://jbbs\.shitaraba\.net/bbs/read\.cgi/(\w+)/(\d+)/(\d+)(:?|\/.*)\z}
SHITARABA_BOARD_URL_PATTERN = %r{\Ahttp://jbbs\.shitaraba\.net/(\w+)/(\d+)/?\z}

def shitaraba_thread?(url)
  !!(url.to_s =~ SHITARABA_THREAD_URL_PATTERN)
end
module_function :shitaraba_thread?

def shitaraba_board?(url)
  !!(url.to_s =~ SHITARABA_BOARD_URL_PATTERN)
end
module_function :shitaraba_board?

def thread_from_url(url)
  if url.to_s =~ SHITARABA_THREAD_URL_PATTERN
    category, board_num, thread_num = $1, $2.to_i, $3.to_i
    board = Board.new(category, board_num)
    thread = board.thread(thread_num)
    raise 'no such thread' if thread.nil?
    return thread
  end
  raise 'bad URL'
end
module_function :thread_from_url

def board_from_url(url)
  if url.to_s =~ SHITARABA_BOARD_URL_PATTERN
    category, board_num = $1, $2.to_i
    board = Board.new(category, board_num)
    return board
  end
  raise 'bad URL'
end
module_function :board_from_url

class Board
  attr_reader :category, :board_num

  def initialize(category, board_num)
    @category = category
    @board_num = board_num
    @settings_url = URI.parse( "http://jbbs.shitaraba.net/bbs/api/setting.cgi/#{category}/#{board_num}/" )
    @thread_list_url = URI.parse( "http://jbbs.shitaraba.net/#{category}/#{board_num}/subject.txt" )
  end

  def dat_url(thread_num)
    return URI.parse("http://jbbs.shitaraba.net/bbs/rawmode.cgi/#{@category}/#{@board_num}/#{thread_num}/")
  end

  def settings
    str = download(@settings_url)
    # 成功した時は EUC-JP で、エラーの時は UTF-8 で来る
    begin
      str.force_encoding("EUC-JP").encode!("UTF-8")
    rescue Encoding::InvalidByteSequenceError
      str.force_encoding("UTF-8")
    end
    return parse_settings(str)
  end

  def thread_list
    str = download(@thread_list_url)
    return str.force_encoding("EUC-JP").encode("UTF-8")
  end

  def dat(thread_num)
    url = dat_url(thread_num)
    str = download(url)
    return str.force_encoding("EUC-JP").encode("UTF-8")
  end

  def thread(thread_num)
    threads.find { |t| t.id == thread_num }
  end

  def threads
    thread_list.each_line.to_a.uniq.map do |line|
      fail 'スレ一覧のフォーマットが変です' unless line =~ /^(\d+)\.cgi,(.+?)\((\d+)\)$/
      id, title, last = $1.to_i, $2, $3.to_i
      Thread.new(self, id, title, last)
    end
  end

  def download(url)
    response = Net::HTTP.start(url.host, url.port) { |http|
      http.get(url.path)
    }
    if response.code != '200'
      raise HTTPError, response.code
    end
    return response.body
  end

  private

  def parse_settings(string)
    string.each_line.map { |line|
      line.chomp.split(/=/, 2)
    }.to_h
  end
end

class Post
  attr_reader :no, :name, :mail, :body

  def self.from_line(line)
    no, name, mail, date, body, = line.split('<>', 6)
    Post.new(no, name, mail, date, body)
  end

  def initialize(no, name, mail, date, body)
    @no = no.to_i
    @name = name
    @mail = mail
    @date = date
    @body = body
  end

  def date
    str2time(@date)
  end

  def date_str
    @date
  end

  def to_s
    [no, name, mail, @date, body, '', ''].join('<>')
  end

  def deleted?
    @date == '＜削除＞'
  end

  private

  def str2time(str)
    if str =~ %r{^(\d{4})/(\d{2})/(\d{2})\(.\) (\d{2}):(\d{2}):(\d{2})$}
      y, mon, d, h, min, sec = [$1, $2, $3, $4, $5, $6].map(&:to_i)
      Time.new(y, mon, d, h, min, sec)
    else
      fail ArgumentError, "#{str.inspect}"
    end
  end
end

class Thread
  attr_reader :id, :title, :last, :board

  def initialize(board, id, title, last = 1)
    @board = board
    @id = id
    @title = title
    @last = last
  end

  def created_at
    Time.at(@id)
  end

  def dat_url
    @board.dat_url(@id)
  end

  def posts(range)
    fail ArgumentError unless range.is_a? Range
    dat_for_range(range).each_line.map do |line|
      Post.from_line(line.chomp).tap do |post|
        # ついでに last を更新
        @last = [post.no, last].max
      end
    end
  end

  def dat_for_range(range)
    if range.last == Float::INFINITY
      query = "#{range.first}-"
    else
      query = "#{range.first}-#{range.last}"
    end
    url = URI(dat_url + query)
    @board.download(url).force_encoding("EUC-JP").encode("UTF-8")
  end
end

end # Module
# include Bbs
# my_board = Board.new("game", 48538)
# # puts my_board.settings
# # puts my_board.thread_list
# t =  my_board.thread(1416739363)

# p t.posts(900..950)
