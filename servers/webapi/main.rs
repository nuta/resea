use resea::idl::{self, net::call_tcp_listen};
use resea::prelude::*;
use resea::server::connect_to_server;

const WEBAPI_PORT: u16 = 80;
static MEMMGR_SERVER: Channel = Channel::from_cid(1);
static _KERNEL_SERVER: Channel = Channel::from_cid(2);

const INDEX_HTML: &str = r#"
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Resea Web API</title>
</head>
<body>
<h1>Resea Web API</h1>
<hr>
<ul>
    <li><a href="/status">GET /status</a>
    <li><a href="/xkcd">GET /xkcd</a>
</ul>
<hr>
<address>Resea Web API Server</address>
</body>
</html>
"#;

struct Server {
    ch: Channel,
    net_server: Channel,
}

impl Server {
    pub fn new(net_server: Channel) -> Server {
        let server_ch = Channel::create().unwrap();
        let client_ch = Channel::create().unwrap();
        client_ch.transfer_to(&server_ch).unwrap();
        call_tcp_listen(&net_server, client_ch, WEBAPI_PORT).unwrap();
        Server {
            ch: server_ch,
            net_server,
        }
    }

    pub fn handle_request(&mut self, method: &str, path: &str) -> String {
        info!("{} {}", method, path);
        let ok = "200 OK";
        let html = "text/html";
        let json = "application/json";

        let (status, response) = match (method, path) {
            ("GET", "/") => {
                (ok, Some((html, INDEX_HTML.to_owned())))
            }
            ("GET", "/status") => {
                (ok, Some((json, r#"{ "status": "working" }"#.to_owned())))
            }
            ("GET", "/xkcd") => {
                let body = r#"
                    <html>
                        <script>
                            window.open("https://xkcd.com/1508", "_self")
                        </script>
                    </html>
                    "#;
                (ok, Some((html, body.to_owned())))
            }
            (_, _) => ("400 Bad Request", None),
        };

        if let Some((content_type, body)) = response {
            format!(concat!(
                "HTTP/1.1 {}\r\n",
                "Server: Resea WebAPI server\r\n",
                "Connection: close\r\n",
                "Content-Type: {}\r\n",
                "Content-Length: {}\r\n",
                "\r\n",
                "{}"
            ), status, content_type, body.len(), body)
        } else {
            format!(concat!(
                "HTTP/1.1 {}\r\n",
                "Server: Resea WebAPI server\r\n",
                "Connection: close\r\n",
                "\r\n",
            ), status)
        }
    }
}

impl idl::net_client::Server for Server {
    fn tcp_received(&mut self, _from: &Channel, client_sock: HandleId, data: Page) {
        info!("tcp_receive");
        let req = resea::str::from_utf8(data.as_bytes()).unwrap_or("");

        if req.contains("\r\n\r\n") {
            let request_line = &req[..req.find('\n').unwrap_or(req.len())];
            let mut words = request_line.split(' ');
            let method = words.next().unwrap_or("GET");
            let path = words.next().unwrap_or("/");
            let resp = self.handle_request(method, path);
            info!("resp = {}", resp);
            use resea::PAGE_SIZE;
            use resea::utils::align_up;
            let num_pages = align_up(resp.len(), PAGE_SIZE) / PAGE_SIZE;
            let mut page = idl::memmgr::call_alloc_pages(&MEMMGR_SERVER, num_pages).unwrap();
            page.copy_from_slice(resp.as_bytes());
            use idl::net::call_tcp_write;
            use idl::net::call_tcp_close;
            call_tcp_write(&self.net_server, client_sock, page).unwrap();
            call_tcp_close(&self.net_server, client_sock).unwrap();
        }
    }

    fn tcp_accepted(&mut self, _from: &Channel, _listen_sock: HandleId, _client_sock: HandleId) {
        info!("tcp_accept");
    }
}

impl resea::mainloop::Mainloop for Server {}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let net_server = connect_to_server(idl::net::INTERFACE_ID)
        .expect("failed to connect to net server");
    let mut server = Server::new(net_server);
    info!("ready");
    mainloop!(&mut server, [net_client]);
}
