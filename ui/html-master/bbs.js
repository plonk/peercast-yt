var THREAD_URL_PATTERN = /^(\w+):\/\/([^\/]+)\/(test|bbs)\/read\.cgi\/(\w+)\/(\d+)(\/(\d+))?(:?|\/.*)$/;
var BOARD_URL_PATTERN = /^(\w+):\/\/([^\/]+)\/(\w+)(\/(\d+))?\/?$/;

function renderPost(p, url) {
    var buf = "";
    buf += "<dt><a target='_blank' href='"+threadUrl(url)+"/"+p.no+"'>"+p.no+"</a>：";
    if (p.mail !== "")
        buf += "<a href='mailto:"+p.mail+"'>";
    buf += "<b>"+p.name+"</b>";
    if (p.mail !== "")
        buf += "</a>";
    buf += "："+p.date+"</dt>";
    buf += "<dd>" + p.body + "</dd>";
    return buf;
}

function boardCgi(url) {
    return ("/cgi-bin/board.cgi?fqdn="+url.fqdn+"&category="+url.category+"&board_num="+url.board_num);
}

function threadCgi(url, id) {
    return ("/cgi-bin/thread.cgi?fqdn="+url.fqdn+"&category="+url.category+"&board_num="+url.board_num+"&id="+id);
}

function boardUrl(url) {
    return (url.protocol+"://"+url.fqdn+"/"+url.category+"/"+url.board_num);
}

function threadUrl(url) {
    return (url.protocol+"://"+url.fqdn+"/"+(url.shitaraba ? "bbs" : "test")+"/read.cgi/"+url.category+(url.shitaraba ? "/"+url.board_num+"/" : "/")+url.thread_id);
}

function newPostsCallback(url, thread) {
    var dl = $('#bbs-view > dl.thread');

    for (var i = 0; i < thread.posts.length; i++) {
        var p = thread.posts[i];
        var elements = $(renderPost(p, url));
        dl.append($(elements).hide().show('highlight'));
    }

    // 最下部にスクロール。
    if (thread.posts.length > 0) {
        var view = $('#bbs-view');
        view.animate({ scrollTop: view.prop("scrollHeight") - view.height() }, 0);
    }

    // 次のレス取得をスケジュールする。
    setTimeout(function () {
        $.getJSON(threadCgi(url, thread.id)+"&first="+(thread.last+1),
                  function (thread) {
                      console.log(thread);
                      newPostsCallback(url, thread);
                  });
    }, 7 * 1000);
}

function threadLinkCallback(url, board_title) {
    $.getJSON(threadCgi(url, url.thread_id), function (thread) {
        console.log(thread);
        var buf = "";

        var board_url = boardUrl(url);
        var thread_url = threadUrl(url);
        $('#bbs-title').html("<a class='board-link' href='"+board_url+"' data-protocol='"+url.protocol+"' data-fqdn='"+url.fqdn+"' data-category='"+url.category+"' data-board_num='"+url.board_num+"'>"+h(board_title)+"</a> &raquo; <b>"+"<a target='_blank' href='"+thread_url+"/l5#form_write'>"+h(thread.title)+"</a></b>");

        $('.board-link').on('click', function (e) {
            if (e.button === 0) {
                console.log(e.button);
                e.preventDefault();
                boardLinkCallback($(this).data('protocol'), $(this).data('fqdn') ,$(this).data('category'), $(this).data('board_num') );
            }
        });

        buf += "<dl class='thread'>";
        buf += "</dl>";
        $('#bbs-view').html(buf);

        newPostsCallback(url, thread);
    });
}

function boardLinkCallback(protocol, fqdn, category, board_num) {
    $('#bbs-view').text("掲示板を読み込み中…");
    var url = match_board(["", protocol, fqdn, category, "", board_num]);
    $.getJSON(boardCgi(url)).done(function (board) {
        console.log(board);

        var board_url = boardUrl(url);
        $('#bbs-title').html("<a target='_blank' href='"+board_url+"'><b>"+h(board.title)+"</b></a>");

        var buf = "";
        for (var i = 0; i < board.threads.length; i++) {
            var t = board.threads[i];
            var thread_url = url.protocol+"://"+url.fqdn+"/"+(url.shitaraba ? "bbs" : "test")+"/read.cgi/"+url.category+(url.shitaraba ? "/"+url.board_num+"/" : "/")+t.id+"/l50";
            buf += "<a href='"+thread_url+"' class='thread-link' data-thread-id="+t.id+">"+h(t.title)+" ("+t.last+")</a><br>";
        }
        $('#bbs-view').html(buf);

        $('.thread-link').on('click', function (e) {
            if (e.button === 0) {
                
                url.thread_id = $(this).data("thread-id");
                threadLinkCallback(url, board.title);
                e.preventDefault();
            }
        });
    }).fail(function () {
        $('#bbs-view').text("エラー: /cgi-bin/board.cgiの実行に失敗しました。");
    });
}

function h(str) {
    var table = {
        '&': "&amp;",
        '<': "&lt;",
        '>': "&gt;",
        '\"': "&quot;",
        '\'': "&#39;",
    };
    return str.replace(/[&<>"']/g, function (char) {
        return table[char];
    });
}

function match_board(match) {
    var url = {};
    url.protocol = match[1];
    url.fqdn = match[2];
    url.shitaraba = ~match[2].indexOf("shitaraba");
    url.category = match[3];
    url.board_num = url.shitaraba ? match[5] : "";
    return url;
}

function match_thread(match) {
    var url = {};
    url.protocol = match[1];
    url.fqdn = match[2];
    url.shitaraba = ~match[2].indexOf("shitaraba");
    url.category = match[4];
    url.board_num = url.shitaraba ? match[5] : "";
    url.thread_id = url.shitaraba ? match[7] : match[5];
    return url;
}

$(function(){
    // チャット表示切り替えチェックボックスの挙動。
    $('#chat-visibility').on('change', function() {
        if ($(this).prop("checked")) {
            $('#bbs-view').show(400);
        } else {
            $('#bbs-view').hide(400);
        }
    });

    var match;
    if (match = BOARD_URL_PATTERN.exec(CONTACT_URL)) {
        // スレッドリストを取得する。
        boardLinkCallback(match[1], match[2], match[3], match[5] ? match[5] : "");
    } else if (match = THREAD_URL_PATTERN.exec(CONTACT_URL)) {
        var url = match_thread(match);
        $.getJSON("/cgi-bin/board.cgi?fqdn="+url.fqdn+"&category="+url.category+"&board_num="+url.board_num).done(function (board) {
                threadLinkCallback(url, board.title);
            });
    } else if (CONTACT_URL === "") {
        $("#bbs-title").text("n/a");
        $('#bbs-view').html("コンタクトURLがありません。");
    } else {
        $("#bbs-title").text("n/a");
        $('#bbs-view').html("対応した掲示板のURLではありません:<br>" + h(CONTACT_URL));
    }
});
