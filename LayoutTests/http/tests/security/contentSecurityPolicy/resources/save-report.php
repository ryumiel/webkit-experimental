<?php
require_once "report-file-path.php";

function undoMagicQuotes($value) {
    if (get_magic_quotes_gpc())
        return stripslashes($value);
    return $value;
}

$reportFile = fopen($reportFilePath . ".tmp", 'w');
$httpHeaders = $_SERVER;
ksort($httpHeaders, SORT_STRING);
foreach ($httpHeaders as $name => $value) {
    if ($name === "CONTENT_TYPE" || $name === "HTTP_REFERER" || $name === "REQUEST_METHOD") {
        $value = undoMagicQuotes($value);
        fwrite($reportFile, "$name: $value\n");
    }
}
fwrite($reportFile, "=== POST DATA ===\n");
fwrite($reportFile, file_get_contents("php://input"));
fclose($reportFile);
rename($reportFilePath . ".tmp", $reportFilePath);
?>
