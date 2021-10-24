# -*- coding: utf-8 -*-
import configparser, re, urllib.request, html

def print_bad_request(message):
  print("Status: 400 Bad Request")
  print("Content-Type: text/plain")
  print("")
  print(message)

class Board:

  def __init__(self, fqdn, category, board_num):
    self.fqdn = fqdn
    self.shitaraba = "jbbs.shitaraba.net" in fqdn
    self.category = category
    self.board_num = board_num
    self.resmax = 1000
    self.urlpath = category + "/" + board_num if len(board_num) != 0 else category

    self.__settings_url = ("http://{0}/bbs/api/setting.cgi/{1}" if self.shitaraba else "http://{0}/{1}/SETTING.TXT").format(self.fqdn, self.urlpath)
    self.__thread_list_url = "http://{0}/{1}/subject.txt".format(self.fqdn, self.urlpath)
    self.external_encoding = "EUC-JP" if self.shitaraba else "CP932"

  def dat_url(self, thread_num):
    return ("http://{0}/bbs/rawmode.cgi/{1}/{2}/" if self.shitaraba else "http://{0}/{1}/dat/{2}.dat").format(self.fqdn, self.urlpath, thread_num)

  def settings(self):
    try:
      str = self.download(self.__settings_url)
    except urllib.error.HTTPError:
      config = configparser.ConfigParser()
      config.read_string("[DEFAULT]\n" + "ERROR = [Settings download error]\n")
      return config.defaults()
    try:
      str = str.decode(self.external_encoding)
    except:
      str = str.decode("UTF-8")
    return self.__parse_settings(str)

  def thread_list(self):
    try:
      str = self.download(self.__thread_list_url)
    except urllib.error.HTTPError:
      # したらばでスレッドがまだないときはsubject.txtが404になる。
      str = b""
    return str.decode(self.external_encoding)

  def thread(self, thread_num):
    return next((t for t in self.threads() if t.id == thread_num), None)

  def threads(self):
    threads = []
    lines = self.thread_list().splitlines()
    p = re.compile("^(\d+)\.cgi,(.+?)\((\d+)\)$" if self.shitaraba else "^(\d+)\.dat<>(.+?)\s\((\d+)\)$")
    for i, line in enumerate(lines):
      if line == "": # したらばでスレッドがない場合、空行だけになる。
        continue
      m = p.match(line)
      self.resmax = max(self.resmax, int(m.group(3)))
      threads.append(Thread(self, m.group(1), html.unescape(m.group(2)), m.group(3)))
    if self.shitaraba and len(threads) > 1:
      threads.pop(len(threads) - 1)
    return threads

  def download(self, url):
    response = urllib.request.urlopen(url)
    return response.read()

  def __parse_settings(self, string):
    config = configparser.ConfigParser()
    config.read_string("[DEFAULT]\n" + string)
    return config.defaults()

class Post:

  @classmethod
  def from_line(cls, line, shitaraba):
    if shitaraba:
      lines = line.split('<>', 6)
      return cls(lines[0], lines[1], lines[2], lines[3], lines[4])
    else:
      lines = line.split('<>', 4)
      return cls(0, lines[0], lines[1], lines[2], lines[3])

  def __init__(self, no, name, mail, date, body):
    self.no = int(no)
    self.name = name
    self.mail = mail
    self.date = date
    self.body = body

class Thread:

  def __init__(self, board, id, title, last = 1):
    self.board = board
    self.id = id
    self.title = title
    self.last = int(last)

  def dat_url(self):
    return self.board.dat_url(self.id)

  def posts(self, r):
    lines = self.dat_for_range(r).splitlines()
    for i, line in enumerate(lines):
      lines[i] = Post.from_line(line, self.board.shitaraba)
      if lines[i].no == 0:
        lines[i].no = i+r.start
      self.last = max(lines[i].no, self.last)
    return lines

  def dat_for_range(self, r):
    if self.board.shitaraba:
      if r.stop >= self.board.resmax:
        query = "{0}-".format(r.start)
      else:
        query = "{0}-{1}".format(r.start, r.stop)
      url = self.dat_url() + query
      return self.board.download(url).decode(self.board.external_encoding)
    else:
      url = self.dat_url()
      lines = self.board.download(url).decode(self.board.external_encoding).splitlines()
      return "".join(map(lambda line: line + "\n", lines[r.start-1:]))
