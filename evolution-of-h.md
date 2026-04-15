Heh. So for my raspberry-pi CGI jank, stdout is the body of the http response.

Originally my "webserver scripts" were just echoing the results they wanted:
```sh
echo '<html lang=en>'
echo '<head>'
echo '  <title>Hello $place!</title>'
echo '</head>'
echo '<body>'
echo "  <h1>Hello, $place!</h1>"
echo "  <p>The time on the server is currently $(date).</p>"
echo "  <p>It's a good day today!</p>"
echo '</body>'
echo '</html>'
```
That tiresome fast, so I made an `h` script which abstracted a lot of it away:
```sh
h html -lang=en
h head
  h title Hello $place!
h /head
h body
  h h1 Hello $place!
  h p The time on the server is currently $(date).
  h p It\'s a good day today!
h /body
h /html
```
But I _still_ wasn't satisfied with that—I wanted to do `h /body /html` at a minimum. Then it dawned on me—`h` could be recursive. Here's what it looks like now:
```sh
h html -a lang=en [ \
  head [ title -t "Hello $place" ] \
  body [ \
    h1 -t "Hello $place!" \
    p -t "The time on the server is currently $(date)." \
      -t "It's a good day today!" \
  ] \
]
```
So much better!
