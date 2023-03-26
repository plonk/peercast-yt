// -*- mode: js; js-indent-level: 4 -*-

(function(){
    function ASSERT_EQ(value, expectation, description) {
        if (JSON.stringify(value) !== JSON.stringify(expectation)) {
            console.error("expected", JSON.stringify(expectation), "\nbut got", JSON.stringify(value))
            throw new Error(`Assertion '${description}' failed`)
        }
    }

    function ASSERT_TRUE(value, description) {
        if (!value) {
            throw new Error(`Assertion '${description}' failed`)
        }
    }

    // getBoardInfo
    ASSERT_EQ(getBoardInfo("http://pcgw.pgw.jp/shuuraku/"),
              { protocol: "http", fqdn: "pcgw.pgw.jp", shitaraba: false, category: "shuuraku", board_num: "" },
             'getBoardInfo genkai')

    ASSERT_EQ(getBoardInfo("https://jbbs.shitaraba.net/computer/44871/"),
              { protocol: "https", fqdn: "jbbs.shitaraba.net", shitaraba: true, category: "computer", board_num: "44871" },
             'getBoardInfo shitaraba')

    const genkai_info = getBoardInfo('http://genkai.pcgw.pgw.jp/shuuraku/')
    genkai_info['thread_id'] = '123456789'

    const shitaraba_info = getBoardInfo("https://jbbs.shitaraba.net/computer/44871/")
    shitaraba_info['thread_id'] = '123456789'

    const post = { no: '1', name: 'nanasi', mail: 'mail', date: '1970-01-01 00:00:00', body: 'body' }

    // renderPost
    ASSERT_EQ(renderPost(post, genkai_info),
              "<div class='highlight' style='padding: 2px; font-size: 13px'><a target='_blank' title='nanasi mail 1970-01-01 00:00:00' href='http://genkai.pcgw.pgw.jp/test/read.cgi/shuuraku/123456789/1'><b>1</b></a>：body</div>",
             'renderPost')

    // boardCgi
    ASSERT_EQ(boardCgi(genkai_info), "/cgi-bin/board.cgi?fqdn=genkai.pcgw.pgw.jp&category=shuuraku&board_num=",
             'boardCgi genkai')

    ASSERT_EQ(boardCgi(shitaraba_info), "/cgi-bin/board.cgi?fqdn=jbbs.shitaraba.net&category=computer&board_num=44871",
             'boardCgi shitaraba')

    // threadCgi
    ASSERT_EQ(
        threadCgi(genkai_info, '123456789'),
        "/cgi-bin/thread.cgi?fqdn=genkai.pcgw.pgw.jp&category=shuuraku&board_num=&id=123456789",
        'threadCgi genkai')

    ASSERT_EQ(
        threadCgi(shitaraba_info, '123456789'),
        "/cgi-bin/thread.cgi?fqdn=jbbs.shitaraba.net&category=computer&board_num=44871&id=123456789",
        'threadCgi shitaraba')

    // boardUrl
    ASSERT_EQ(
        boardUrl(genkai_info),
        "http://genkai.pcgw.pgw.jp/shuuraku/",
        'boardUrl genkai')

    ASSERT_EQ(
        boardUrl(shitaraba_info),
        "https://jbbs.shitaraba.net/computer/44871",
        'boardUrl shitaraba')

    // threadUrl
    ASSERT_EQ(
        threadUrl(genkai_info),
        "http://genkai.pcgw.pgw.jp/test/read.cgi/shuuraku/123456789",
        'threadUrl genkai')

    ASSERT_EQ(
        threadUrl(shitaraba_info),
        "https://jbbs.shitaraba.net/bbs/read.cgi/computer/44871/123456789",
        'threadUrl shitaraba')

    // h()
    ASSERT_EQ(h(`abc&<>"'`), "abc&amp;&lt;&gt;&quot;&#39;", 'h')

    // getThreadInfo
    ASSERT_EQ(getThreadInfo("http://pcgw.pgw.jp/test/read.cgi/shuuraku/123456789"),
              { protocol: "http", fqdn: "pcgw.pgw.jp", shitaraba: false, category: 'shuuraku', board_num: "", thread_id: "123456789" },
             'getThreadInfo genkai')

    ASSERT_EQ(getThreadInfo("https://jbbs.shitaraba.net/bbs/read.cgi/computer/44871/123456789"),
              { protocol: "https", fqdn: "jbbs.shitaraba.net", shitaraba: true, category: 'computer', board_num: "44871", thread_id: "123456789" },
             'getThreadInfo shitaraba')

    // delay()
    ASSERT_TRUE(delay(100) instanceof Promise, 'delay return a Promise') 

    // encodeForm()
    ASSERT_EQ(
        encodeForm( { a: "1", b: "2" } ),
        "a=1&b=2",
        'encodeForm')

    console.log("test done")
})()
