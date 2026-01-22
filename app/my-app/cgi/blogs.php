<?php
// Simple script: fetch posts from an API and return N random posts as JSON

$apiUrl = 'https://jsonplaceholder.typicode.com/posts'; // replace with your API
$count = 3; // number of random posts to return

function fetchJson(string $url, int $timeout = 5): ?array {
    $ch = curl_init($url);
    curl_setopt_array($ch, [
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_FOLLOWLOCATION => true,
        CURLOPT_CONNECTTIMEOUT => $timeout,
        CURLOPT_TIMEOUT => $timeout,
        CURLOPT_USERAGENT => 'php-random-posts/1.0',
    ]);
    $body = curl_exec($ch);
    $err  = curl_error($ch);
    $code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);

    if ($body === false || $code >= 400) return null;
    $data = json_decode($body, true);
    return is_array($data) ? $data : null;
}

$posts = fetchJson($apiUrl);
if ($posts === null || count($posts) === 0) {
    http_response_code(502);
    header('Content-Type: application/json');
    echo json_encode(['error' => 'Failed to fetch posts from upstream API']);
    exit;
}

// pick N random unique posts
$count = max(1, min($count, count($posts)));
$keys = array_rand($posts, $count);
if ($count === 1) {
    $random = [$posts[$keys]];
} else {
    $random = [];
    foreach ($keys as $k) $random[] = $posts[$k];
}

// output JSON
header('Content-Type: application/json; charset=utf-8');
echo json_encode(array_values($random), JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT);