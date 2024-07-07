from http import server


class Handler(server.SimpleHTTPRequestHandler):

    def end_headers(self):
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        super().end_headers()


if __name__ == "__main__":
    server.test(HandlerClass=Handler)
