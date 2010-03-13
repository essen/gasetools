<?php

// Gets a filename list of all the valid .nbl files in the extracted data

$sDataPath = '/home/essen/dev/psu/aotioffline/data/';
$sDestPath = 'data';

@mkdir($sDestPath);

$oIterator = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($sDataPath));
foreach ($oIterator as $sFilename => $oFile) {
	if ($oFile->isDir())
		continue;

	$sDir = $sDestPath . strrchr($oFile->getPath(), '/') . '/' . $oFile->getFilename();
	@mkdir($sDir, 0777, true);
	system('./nbl -o ' . $sDir . ' ' . $sFilename);
}
