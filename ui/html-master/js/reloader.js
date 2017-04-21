$(function () {
    var reloaders = $('.reloader');

    for (var i = 0; i < reloaders.length; i++) {
        (function (r) {
            var callback = function () {
                $.ajax(r.dataset.url, {
                    cache: false,
                    success: function (data) {
                        $(r).html(data);
                        setTimeout(callback, r.dataset.interval * 1000);
                    },
                    error: function (req) {
                        $(r).html(req.responseText);
                        setTimeout(callback, r.dataset.interval * 1000);
                    },
                });
            };
            setTimeout(callback, r.dataset.interval * 1000);
        })(reloaders[i]);
    }
});
