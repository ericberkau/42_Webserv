<?php
header("Content-Type: text/plain");

echo "Hello from PHP CGI!\n";
echo "PHP version: " . PHP_VERSION . "\n";
echo "Request method: " . ($_SERVER["REQUEST_METHOD"] ?? "unknown") . "\n";
echo "Script executed successfully.\n";
?>