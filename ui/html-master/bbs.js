var SHITARABA_THREAD_URL_PATTERN = /^http:\/\/jbbs\.shitaraba\.net\/bbs\/read\.cgi\/(\w+)\/(\d+)\/(\d+)(:?|\/.*)$/;
var SHITARABA_BOARD_URL_PATTERN = /^http:\/\/jbbs\.shitaraba\.net\/(\w+)\/(\d+)\/?$/;

function renderPost(p, category, board_num, thread_id) {
    var buf = "";
    var url = "http://jbbs.shitaraba.net/bbs/read.cgi/"+category+"/"+board_num+"/"+thread_id+"/"+p.no;
    buf += "<dt><a target='_blank' href='"+url+"'>"+p.no+"</a>：";
    if (p.mail !== "")
        buf += "<a href='mailto:"+p.mail+"'>";
    buf += "<b>"+p.name+"</b>";
    if (p.mail !== "")
        buf += "</a>";
    buf += "："+p.date+"</dt>";
    buf += "<dd>" + p.body + "</dd>";
    return buf;
}

function newPostsCallback(posts, category, board_num, thread) {
    var dl = $('#bbs-view > dl.thread');

    for (var i = 0; i < posts.length; i++) {
        var p = posts[i];
        var elements = $(renderPost(p, category, board_num, thread.id));
        dl.append($(elements).hide().show('highlight'));
    }

    // 最下部にスクロール。
    if (posts.length > 0) {
        var view = $('#bbs-view');
        view.animate({ scrollTop: view.prop("scrollHeight") - view.height() }, 0);
    }

    // 次のレス取得をスケジュールする。
    setTimeout(function () {
        $.getJSON("/cgi-bin/thread.cgi?category="+category+"&board_num="+board_num+"&id="+thread.id+"&first="+(thread.last+1),
                  function (thread) {
                      console.log(thread);
                      newPostsCallback(thread.posts, category, board_num, thread);
                  });
    }, 7 * 1000);
}

function threadLinkCallback(category, board_num, board_title, thread_id) {
    var url = "/cgi-bin/thread.cgi?category="+category+"&board_num="+board_num+"&id="+thread_id;
    console.log(url);
    $.getJSON(url, function (thread) {
        console.log(thread);
        var posts = thread.posts;
        var buf = "";

        $('#bbs-title').html("<span class='board-link' data-category='"+category+"' data-board_num='"+board_num+"'>"+h(board_title)+"</span> &raquo; <b>"+h(thread.title)+"</b>");

        $('.board-link').on('click', function () {
            boardLinkCallback($(this).data('category'), +($(this).data('board_num')));
        });

        buf += "<dl class='thread'>";
        buf += "</dl>";
        $('#bbs-view').html(buf);

        newPostsCallback(posts, category, board_num, thread);
    });
}

function boardLinkCallback(category, board_num) {
    $('#bbs-view').text("掲示板を読み込み中…");
    $.getJSON("/cgi-bin/board.cgi?category="+category+"&board_num="+board_num).done(function (board) {
        console.log(board);

        $('#bbs-title').html("<b>"+board.title+"</b>");

        var buf = "";
        for (var i = 0; i < board.threads.length; i++) {
            var t = board.threads[i];
            buf += "<span class='thread-link' data-thread-id="+t.id+">"+t.title+" ("+t.last+")</span><br>";
        }
        $('#bbs-view').html(buf);

        $('.thread-link').on('click', function () {
            threadLinkCallback(board.category, board.board_num, board.title, $(this).data("thread-id"));
        });
    }).fail(function () {
        console.log('fail');
    });
}

// unimplemented
function h(str) {
    return str;
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
    if (match = SHITARABA_BOARD_URL_PATTERN.exec(CONTACT_URL)) {
        // スレッドリストを取得する。
        boardLinkCallback(match[1], match[2]);
    } else if (match = SHITARABA_THREAD_URL_PATTERN.exec(CONTACT_URL)) {
        $.getJSON("/cgi-bin/board.cgi?category="+match[1]+"&board_num="+match[2]).done(function (board) {
            threadLinkCallback(match[1], match[2], board.title, match[3]);
        });
    } else if (CONTACT_URL === "") {
        $("#bbs-title").text("n/a");
        $('#bbs-view').html("コンタクトURLがありません。");
    } else {
        $("#bbs-title").text("n/a");
        $('#bbs-view').html("対応した掲示板のURLではありません。");
    }
});
