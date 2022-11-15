// -*- mode: js; js-indent-level: 4 -*-

// メイン処理に開いてほしいコマンドを入れるキュー。
// ['open', URL] スレッド、板を開く。
// ['post', MESSAGE] 投稿。
$queue = [];

function renderPost(p, info) {
    var buf = "";
    buf += "<div class='highlight' style='padding: 2px; font-size: 13px'>";
    buf += `<a target='_blank' href='${threadUrl(info)}/${p.no}'><b>${p.no}</b></a>：${p.body}`;
    buf += "</div>"
    return buf;
}

function boardCgi(info) {
    return ("/cgi-bin/board.cgi?fqdn="+info.fqdn+"&category="+info.category+"&board_num="+info.board_num);
}

function threadCgi(info, id) {
    return ("/cgi-bin/thread.cgi?fqdn="+info.fqdn+"&category="+info.category+"&board_num="+info.board_num+"&id="+id);
}

function boardUrl(info) {
    return (info.protocol+"://"+info.fqdn+"/"+info.category+"/"+info.board_num);
}

function threadUrl(info) {
    return (info.protocol+"://"+info.fqdn+"/"+(info.shitaraba ? "bbs" : "test")+"/read.cgi/"+info.category+(info.shitaraba ? "/"+info.board_num+"/" : "/")+info.thread_id);
}

function newPostsCallback(url, thread) {
    var div = $('#bbs-view > div.thread');

    for (var i = 0; i < thread.posts.length; i++) {
        var p = thread.posts[i];
        var elements = $(renderPost(p, url));

        div.append(elements);
    }

    // 最下部にスクロール。
    if (thread.posts.length > 0) {
        var view = $('#bbs-view');
        view.animate({ scrollTop: view.prop("scrollHeight") - view.height() }, 0);
    }
}

// スレッドが開かれたときにDOMを変える。実際のレスは追加しない。
function loadThread(info, thread, board_title) {
    var buf = "";

    var board_url = boardUrl(info);
    var thread_url = threadUrl(info);
    var naviHtml;
    naviHtml = `<a class='board-link' href='${board_url}'>${h(board_title)}</a>`;
    naviHtml += ' &raquo; ';
    naviHtml += `<b><a target='_blank' href='${thread_url}/l5#form_write'>${h(thread.title)}</a></b>`;
    $('#bbs-title').html(naviHtml);

    $('.board-link').on('click', function (e) {
        if (e.button === 0) {
            e.preventDefault();
            const info = getBoardInfo($(this).attr('href'));
            $queue.push(['open', board_url]);
        }
    });

    buf += "<div class='thread'>";
    buf += "</div>";
    $('#bbs-view').html(buf);
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

function getBoardInfo(url) {
    var match;
    if (match = /^(\w+):\/\/([^\/]+)\/(\w+)(\/(\d+))?\/?$/.exec(url)) {
        var info = {};
        info.protocol = match[1];
        info.fqdn = match[2];
        info.shitaraba = match[2].indexOf("shitaraba") != -1;
        info.category = match[3];
        info.board_num = info.shitaraba ? match[5] : "";
        return info;
    } else {
        return null;
    }
}

function getThreadInfo(url) {
    var match;
    if (match = /^(\w+):\/\/([^\/]+)\/(test|bbs)\/read\.cgi\/(\w+)\/(\d+)(\/(\d+))?(:?|\/.*)$/.exec(url)) {
        var info = {};
        info.protocol = match[1];
        info.fqdn = match[2];
        info.shitaraba = match[2].indexOf("shitaraba") != -1;
        info.category = match[4];
        info.board_num = info.shitaraba ? match[5] : "";
        info.thread_id = info.shitaraba ? match[7] : match[5];
        return info;
    } else {
        return null;
    }
}

// #chat-visibility チェックボックスの状態を見て、#bbs-view-conatiner
// の表示/非表示を切り替える。
function switchChatVisibility() {
    if ($('#chat-visibility').prop("checked")) {
        $('#bbs-view-container').show(400);

        // 最新レスが見えるように最下部にスクロールする。
        var view = $('#bbs-view');
        view.animate({ scrollTop: view.prop("scrollHeight") - view.height() }, 0);
    } else {
        $('#bbs-view-container').hide(400);
    }
}

function postMessage(message) {
    $queue.push(['post', message])
}

function delay(ms) {
    return new Promise((resolve, _reject) => setTimeout(resolve, ms));
}

function encodeForm(obj) {
    let buf = ""
    for (var key of Object.keys(obj)) {
        buf += `&${key}=${encodeURIComponent(obj[key])}`;
    }
    return buf.substring(1);
}

async function doPostMessageAsync(info, body) {
    const name = '', mail = 'sage';
    const query = encodeForm({'fqdn':info.fqdn, 'category':info.category, 'id':info.thread_id, 'board_num':info.board_num}) + '&' + encodeForm({name, mail, body})

    try {
        const result = await $.getJSON(`/cgi-bin/post.cgi?${query}`);
        if (result.status == 'error') {
            alert(`Error: code = ${result.code}`);
        } else {
            $('#message-input').prop('value', '');
        }
    } catch (xhr) {
        alert(`${xhr.status} ${xhr.statusText}`);
    }
}

// url を開く。開けないURLならそのまま終了。ユーザーからの入力で別の
// URLを開く。スレッドURLでは取得ループも同時に行う。
async function mainAsync(url) {
    var info;
    if (info = getBoardInfo(url)) {
        // スレッドリストを取得する。
        $('#bbs-view').text("掲示板を読み込み中…");
        let board;
        try {
            board = await $.getJSON(boardCgi(info));
        } catch {
            $('#bbs-view').text("エラー: /cgi-bin/board.cgiの実行に失敗しました。");
        }
        console.log(board);

        var board_url = boardUrl(info);
        $('#bbs-title').html(`<a target='_blank' href='${board_url}'><b>${h(board.title)}</b></a>`);

        if (board.threads.length > 0) {
            var buf = "";
            for (var i = 0; i < board.threads.length; i++) {
                var t = board.threads[i];
                var thread_url = info.protocol+"://"+info.fqdn+"/"+(info.shitaraba ? "bbs" : "test")+"/read.cgi/"+info.category+(info.shitaraba ? "/"+info.board_num+"/" : "/")+t.id+"/l50";
                buf += `<div style="padding:2px;font-size:13px"><a href='${thread_url}' class='thread-link' data-thread-id=${t.id}>${h(t.title)} (${t.last})</a></div>`;
            }
            $('#bbs-view').html(buf);
        } else {
            $('#bbs-view').html(`掲示板「<a target='_blank' href='${board_url}'>${h(board.title)}</a>」にスレッドはありません。`);
        }

        $('.thread-link').on('click', function (e) {
            if (e.button === 0) {
                $queue.push(['open', $(this).prop('href')]);
                e.preventDefault();
            }
        })

        $('.post-form').hide();

        // 移動要求を待つだけのループ。
        while (true) {
            // 移動要求が来たら中断して全部やりなおす。
            if ($queue.length > 0) {
                const [cmd, arg1] = $queue.shift();
                if (cmd == 'open') {
                    return mainAsync(arg1);
                } else if (cmd == 'post') {
                    alert('post not possible');
                }
            }
            await delay(100);
        }
    } else if (info = getThreadInfo(url)) {
        $('.post-form').show();

        const board = await $.getJSON(`/cgi-bin/board.cgi?fqdn=${info.fqdn}&category=${info.category}&board_num=${info.board_num}`);
        let thread = await $.getJSON(threadCgi(info, info.thread_id));
        console.log(thread);
        loadThread(info, thread, board.title);
        newPostsCallback(info, thread);

        while (true) {
            // 合計７秒間スリープする。
            for (let i = 0; i < 70; i++) {
                // 移動要求が来たら中断して全部やりなおす。
                if ($queue.length > 0) {
                    const [cmd, arg1] = $queue.shift();
                    if (cmd == 'open') {
                        return mainAsync(arg1);
                    } else if (cmd == 'post') {
                        await doPostMessageAsync(info, arg1);
                        break;
                    }
                }
                await delay(100);
            }

            // レスを取得して追加。
            thread = await $.getJSON(threadCgi(info, thread.id)+"&first="+(thread.last+1));
            console.log(thread);
            newPostsCallback(info, thread);
        }
    } else if (url === "") {
        $("#bbs-title").text("n/a");
        $('#bbs-view').html("コンタクトURLがありません。");
    } else {
        $("#bbs-title").text("n/a");
        $('#bbs-view').html("対応した掲示板のURLではありません:<br>" + h(url));
    }
}

function handleSubmit() {
    var value = $('#message-input').prop('value');
    postMessage(value);
}

$(function(){
    switchChatVisibility();

    // チャット表示切り替えチェックボックスの挙動。
    $('#chat-visibility').on('change', function() {
        switchChatVisibility();
    });

    // 投稿ボタンが押された。
    $('#post-button').on('click', handleSubmit);
    $('#message-input').on('keydown',function(ev){
        if (ev.keyCode == 13 && ev.shiftKey) {
            handleSubmit();
            ev.preventDefault();
        }
    });

    mainAsync(CONTACT_URL);
});
