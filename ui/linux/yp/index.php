<!doctype html>
<?php
$CONTEXT = json_decode(getenv("PEERCAST_CONTEXT"));
$channel_count = count($CONTEXT->channels);
?>
<html>
  <head>
    <title>PeerCast Yellow Pages</title>
    <style>
     input, select { font-size: 12px }
     .channel-list tr { background-color: #FFECC4 }
     .channel-list th { font-size: 12px; text-decoration: underline }
     .channel-list td { text-align: center }
     .channel-list td.channel-description { text-align: left  }
     td.results { text-align: left }
     .link { color: blue }
     .em-link { color: blue; font-weight: bold }
     body { background-color: #ddd;
            font-family: Verdana, sans-serif; font-size: 10px; }
     .container { background-color: white;
            width: 700px; margin-left: auto; margin-right: auto;
            border: solid 1px black; padding: 5px }
    </style>
    <meta charset="utf-8">
  </head>
  <body>
    <!-- <pre><?php var_dump($CONTEXT); ?></pre> -->
    <div class="container">
      <center>
        <img width="370" src="peercast-logo.png">
      </center>
      <hr>

      <h2>Local Channels</h2>

      <table width="100%" class="channel-list">
        <tr>
          <td colspan="6" class="results">
            <b>Results:</b>
            <?php echo $channel_count; ?> <?php echo $channel_count == 1 ? "channel" : "channels"; ?>,
            ? listeners,
            ? relays.
          </td>
        </tr>

        <tr>
          <th>Rank</th><th><img src="small-icon.png"></th><th>Channel</th><th>Status</th><th>Bitrate</th><th>Type</th>
        </tr>

        <?php for ($i = 0; $i < count($CONTEXT->channels); $i++) { ?>
        <?php $ch = $CONTEXT->channels[$i]; ?>
        <tr>
          <td><?php echo $i + 1; ?></td>
          <td><img src="play.png" alt="Play"></td>
          <td class="channel-description">
            <span class="em-link"><?php echo $ch->name; ?></span><br>
            <small>[<?php echo $ch->genre; ?>]</small><br>
            Playing: <span style="color: blue"><?php echo $ch->comment; ?></span>
          </td>
          <td>
            <?php echo $ch->hit_stat->listeners; ?> / <?php echo $ch->hit_stat->relays; ?><br>
            <img src="chat.png"> <img src="graph.png">
          </td>
          <td><?php echo $ch->bitrate ?> kb/s</td>
          <td><?php echo $ch->type ?></td>
        </tr>
        <?php } ?>

      </table>

      <hr>
      <h2>Channels from Federated Stations</h2>
      <center style="margin: 0.5em">
        <b>This site is not responsible for the content of these channels</b><br>
      </center>

      <!-- <center style="font-size: 12px; margin: 0.5em"> -->
      <!--   Page: 1 <span style="color: grey">&gt;&gt;&gt;</span> -->
      <!-- </center> -->

    </div>
  </body>
</html>
