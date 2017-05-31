$(function(){
    $('#bbs-view').text("掲示板を読み込み中…");

    // スレッドリストを取得する。
    $.getJSON("/cgi-bin/read.cgi?url="+CONTACT_URL).done(function (data) {
        console.log(data);
        var threads = data.threads;
        var category = data.category;
        var board_num = data.board_num;

        var buf = "";
        for (var i = 0; i < threads.length; i++) {
            var t = threads[i];
            buf += "<span class='thread-link' data-thread-id="+t.id+">"+t.title+" ("+t.last+")</span><br>";
        }
        $('#bbs-view').html(buf);

        $('.thread-link').on('click', function () {
            var url = "/cgi-bin/thread.cgi?category="+category+"&board_num="+board_num+"&id="+$(this).data("thread-id");
            console.log(url);
            $.getJSON(url, function (data) {
                console.log(data);
                var posts = data.posts;
                var buf = "<dl class='thread'>";
                for (var i = 0; i < posts.length; i++) {
                    var p = posts[i];
                    buf += "<dt>"+p.no+"：<b>"+p.name+"</b>"+"："+p.date+"</dt>";
                    buf += "<dd>" + p.body + "</dd>";
                }
                buf += "</dl>";
                $('#bbs-view').html(buf);
            });
        });
    }).fail(function () {
        console.log('fail');
    });
});
