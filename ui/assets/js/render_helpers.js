// HTMLエスケープをする関数。
function h(str)
{
      return str.replace(/[&<>"']/g, (c) => ({'&': "&amp;", '<': "&lt;", '>': "&gt;", '\"': "&quot;", '\'': "&#39;"})[c])
}

// who。HTMLを書くための関数。
const NoCloseTagRequired = { 'img':true, 'br':true, 'meta':true, 'link':true }

function who(...args)
{
      let buf = ""
      let firstTime = true

      for (const arg of args)
      {
          if (firstTime)
              firstTime = false
          else
              buf += ' '

          if (typeof(arg) == 'string')
              buf += h(arg)
          else if (arg instanceof Array)
          {
              let buf2 = "";
              const [elem, attrs, ...inner] = arg
              if (!elem)
                  throw new Error("elem")
              if (elem == '!')
              {
                  buf += [attrs, ...inner].join('')
                  continue;
              }
              buf2 += `<${elem}`
              if (attrs && typeof(attrs) == 'object' && !(attrs instanceof Array))
              {
                  for (const key of Object.keys(attrs))
                  {
                      if (attrs[key] === true)
                          buf2 += ` ${key}`
                      else if (attrs[key] === false || attrs[key] === null || attrs[key] === undefined)
                          buf2 += ''
                      else
                          buf2 += ` ${key}="${h(attrs[key])}"`
                  }
              }else
                  inner.unshift(attrs)
              buf2 += ">"
              buf2 += who(...inner)
              if (!NoCloseTagRequired[elem])
                  buf2 += `</${elem}>`
              buf += buf2
          } else
              throw new Error("Unknown type")
      }
      return buf;
}

// 実体参照を文字にするための関数。タグは除去される。
// HTMLパーズしてテキストだけ取り出す処理が重いので、結果をキャッシュする。
const UHCache = {}
function unescapeHtml(str)
{
    if (UHCache[str] !== undefined) {
        return UHCache[str]
    } else {
        return (UHCache[str] = $($.parseHTML(str)).text())
    }
}

