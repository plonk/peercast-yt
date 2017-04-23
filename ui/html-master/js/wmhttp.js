$(function(){
    function updateOutput() {
        if (!$("#wm_name")[0].value) {
            $("#wm_output_server_name")[0].value = "";
            $("#wm_output_publishing_point")[0].value = "";
            $("#wm_output_url")[0].value = "";
            return;
        }

        var publishing_point = "";
        for (var key of ["name", "genre", "desc", "url"]) {
            if (key !== "name")
                publishing_point += ";";
            publishing_point += $("#wm_"+key)[0].value;
        }
        publishing_point = publishing_point.replace(/\;*$/, '').replace(/\?/g, "");         // '?' is forbidden.
        $("#wm_output_server_name")[0].value = SERVER;
        $("#wm_output_publishing_point")[0].value = publishing_point;
        $("#wm_output_url")[0].value = "http://" + SERVER + "/" + publishing_point;
    }

    $("#wm_name").on('input', updateOutput);
    $("#wm_desc").on('input', updateOutput);
    $("#wm_genre").on('input', updateOutput);
    $("#wm_url").on('input', updateOutput);

    $("#wm_output_server_name").on("focus", function () { $(this).select(); });
    $("#wm_output_publishing_point").on("focus", function () { $(this).select(); });
    $("#wm_output_url").on("focus", function () { $(this).select(); });

    updateOutput();
});
