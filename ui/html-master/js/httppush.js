var SERVER;

$(function(){
    function escape(str) {
        return str.replace(/ /g, "+").replace(/&/g, "%26");
    }
    function buildUrl() {
        if (!$("#name")[0].value) {
            $("#output")[0].value = "";
            return;
        }

        var url = "http://" + SERVER + "/?";
        var params = [];
        for (var key of ["name", "desc", "genre", "contact"]) {
            if ($("#"+key)[0].value)
                params.push( key + "=" + escape($("#"+key)[0].value) );
        }
        params.push("type=" + $("#type")[0].selectedOptions[0].value);
        $("#output")[0].value = "\"" + url + params.join("&") + "\"";
    }
    $("#name").on('input', buildUrl);
    $("#desc").on('input', buildUrl);
    $("#genre").on('input', buildUrl);
    $("#contact").on('input', buildUrl);
    $("#type").on('input', buildUrl);

    $("#output").on("focus", function () {
        $(this).select();
    });
});
