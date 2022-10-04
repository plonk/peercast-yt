async function peercast(method, ... args)
{
    const request = {
        "jsonrpc": "2.0",
        "method": method,
        "params": args,
        "id": null
    }
    const body = JSON.stringify(request)
    const response = await fetch("/api/1", { method: 'POST', body })
    const data = await response.json()
    if (data.error)
        throw new Error(data.error.message);

    return data.result
}
