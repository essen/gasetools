<?php

function f($a, $b) {
	preg_match('/nmll-(\d+).+/', $a, $a1);
	preg_match('/nmll-(\d+).+/', $b, $b1);

	return $a1[1] > $b1[1];
}

$aFiles = glob('*.nbl');
usort($aFiles, 'f');
foreach ($aFiles as $sFilename) {
	if (strpos($sFilename, 'new') !== false)
		continue;

	echo ' * ' . $sFilename . ":\n";
	system('../build/nbl -t ' . $sFilename);
}
