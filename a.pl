print <<END
<html>
<head>
	<meta charset="utf-8">
	<title>VCVRack/miRack module screenshots</title>
</head>
<body>
<p>NOTE: Screenshots rendered in <a href="http://github.com/mi-rack/Rack">miRack</a> and may look slightly different to VCVRack (and in very rare cases may have rendering issues).</p>
END
;

while(<*/>) {
	chop;
	$a = $_;
	print '<h2 style="clear:both;">'.$a.'</h2>'."\n";
	while(<"$a/*.png">) {
		$img = $_;
		$i = `identify "$img"`;
		if ($i =~ /\/(.+)\.png PNG (\d+)x(\d+)/) {
			$n = $1;
			$w = $2 / 2;
			$h = $3 / 2;
			print '<div style="text-align:center;float:left;margin-right:20px;margin-bottom:20px;"><div style="font-weight:bold;">'.$n.'</div><img src="'.$img.'" style="width:'.$w.'px;height:'.$h.'px;"></div>'."\n";
		}
	}
	print "<br><br>\n";
}

print <<END
</body>
</html>
END
;
