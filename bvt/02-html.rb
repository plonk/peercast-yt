require_relative 'common'

spawn_peercast

["127.0.0.1",
 #"[::1]"
].each do |host|
  response = http_get("http://#{host}:7144")
  assert_eq(302, response.code)
  assert_eq('/html/en/index.html', response.headers["Location"])

  response = http_get("http://#{host}:7144/html")
  assert_eq(302, response.code)
  assert_eq('/html/en/index.html', response.headers["Location"])

  response = http_get("http://#{host}:7144/html/")
  assert_eq(404, response.code)

  response = http_get("http://#{host}:7144/html/en")
  assert_eq(404, response.code)

  response = http_get("http://#{host}:7144/html/en/index.html")
  assert_eq(200, response.code)
  assert(response.body.bytesize > 0)

  response = http_get("http://#{host}:7144/html/ja/index.html")
  assert_eq(200, response.code)
  assert(response.body.bytesize > 0)

  response = http_get("http://#{host}:7144/html/index.html")
  assert_eq(302, response.code)
  assert_eq("/", response.headers['Location'])

  files = %w[bcid.html notifications.html settings.html
broadcast.html connections.html login.html viewlog.html
channels.html head.html logout.html relayinfo.html]

  files.each do |file|
    response = http_get("http://#{host}:7144/html/ja/#{file}")
    assert_eq(200, response.code, context: file)
  end

  response = http_get("http://#{host}:7144/html/ja/play.html")
  assert_eq(400, response.code)
end

puts 'ok'
