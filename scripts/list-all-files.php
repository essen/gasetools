<?php

// Gets a filename list of all the valid .nbl files in the extracted data

$sDataPath = '/home/essen/dev/psu/aotioffline/data/';

$oIterator = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($sDataPath));
foreach ($oIterator as $sFilename => $oFile) {
	if ($oFile->isDir())
		continue;

	echo ' * ' . substr(strrchr($oFile->getPath(), '/'), 1) . '/' . $oFile->getFilename() . ":\n";
	system('./nbl -t ' . $sFilename);
}
