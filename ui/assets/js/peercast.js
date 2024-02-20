async function peercast(method, ... args)
{
    const request = {
        "jsonrpc": "2.0",
        "method": method,
        "params": args,
        "id": null
    }
    const body = JSON.stringify(request)
    const headers = { 'X-Requested-With': 'XMLHttpRequest' }
    const response = await fetch("/api/1", { method: 'POST', headers, body })
    if (response.status === 403) {
        // Cookieログインが抜けているのでリロードしてログインフォーム
        // を表示する。
        location.reload()
    }
    const data = await response.json()
    if (data.error) {
        let msg = data.error.message
        if (data.error.data) {
            msg += ": " + JSON.stringify(data.error.data)
        }
        throw new Error(msg)
    }

    return data.result
}
