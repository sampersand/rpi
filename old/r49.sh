#!/bin/sh

test $DEBUG && set -x

set -euC
if test $METHOD = POST; then
  QUERY_STRING=$(cat)
  Kw=`extract-url-encoding "$QUERY_STRING"`
  set -a
  eval "$Kw"
  set +a
fi

if test $METHOD != GET && test $METHOD != POST; then
	status 405 true
	exit
fi

status 200
header Content-Type text/html
cat <<HTML
<!DOCTYPE html>
<html>
<body>
  <form method="POST">
    <label for="code">Code:</label><br>
    <textarea id="code" name="code" rows="5" cols="40">${code-}</textarea><br><br>

    <input type="checkbox" id="include_stderr" name="include_stderr" value="true">
    <label for="include_stderr">Include stderr</label><br><br>

    <input type="submit" value="Submit">
  </form>
HTML

if test $METHOD == POST; then
  echo '<pre><code>'

  set +e
  echo '--response--'
  r49 -e "${code:?}" 2>&1
  exit=$?
  set -e
  echo '--exit code--'
  echo $exit
  test $exit -ne 0 && echo "exit status: $exit"
  exit 0
  echo '</code></pre>'
fi

cat <<HTML
  </body>
</html>
HTML
