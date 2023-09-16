function getCookie(name)
{
  var matches = document.cookie.match(new RegExp(
    "(?:^|; )" + name.replace(/([\.$?*|{}\(\)\[\]\\\/\+^])/g, '\\$1') + "=([^;]*)"
  ));
  return matches ? decodeURIComponent(matches[1]) : undefined;
}

function setCookie(name, value)
{
  value = encodeURIComponent(value);
  var updatedCookie = name + "=" + value;
  document.cookie = updatedCookie;
}

function deleteCookie(name)
{
  setCookie(name, "");
}
