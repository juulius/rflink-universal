"""Bounded, ingress-compatible HTTP/1.1 request-body reader.

Home Assistant ingress may forward browser POST bodies using
Transfer-Encoding: chunked. BaseHTTPRequestHandler does not decode that
format for applications; treating a chunked request as an empty body leaves
chunk-size markers behind to be interpreted as new HTTP requests.

Reject ambiguous framing and malformed streams to avoid request smuggling.
"""
from __future__ import annotations

import re


class BodyReadError(ValueError):
    def __init__(self, status: int, message: str):
        super().__init__(message)
        self.status = status


def read_body(headers, stream, limit: int) -> bytes:
    """Read exactly one HTTP request body, bounded by decoded bytes.

    Unsupported or invalid framing raises BodyReadError. The caller MUST
    close the connection on errors because the stream could be unaligned.
    A missing Content-Length and Transfer-Encoding means an empty body.
    """
    if limit <= 0:
        raise ValueError("Body size limit must be positive")

    transfer = headers.get_all("Transfer-Encoding", [])
    content_lengths = headers.get_all("Content-Length", [])
    if transfer:
        if len(transfer) != 1 or transfer[0].strip().lower() != "chunked":
            raise BodyReadError(400, "Only chunked transfer encoding is supported")
        if content_lengths:
            raise BodyReadError(400, "Content-Length and Transfer-Encoding must not be combined")

        data = bytearray()
        while True:
            line = stream.readline(128)
            if not line.endswith(b"\r\n"):
                raise BodyReadError(400, "Malformed chunk size line")
            token = line[:-2].split(b";", 1)[0].strip()
            if not re.fullmatch(rb"[0-9a-fA-F]{1,16}", token):
                raise BodyReadError(400, "Invalid chunk size")
            size = int(token, 16)
            if size > limit - len(data):
                raise BodyReadError(413, "Request body exceeds size limit")
            if size == 0:
                # The blank line after zero-size chunk is mandatory.
                # Consume bounded optional trailers, so keep-alive works.
                trailer_bytes = 0
                while True:
                    trailer = stream.readline(2048)
                    trailer_bytes += len(trailer)
                    if trailer_bytes > 8192 or not trailer.endswith(b"\r\n"):
                        raise BodyReadError(400, "Malformed or oversized chunk trailers")
                    if trailer == b"\r\n":
                        return bytes(data)
                    if not re.fullmatch(rb"[!#$%&'*+.^_`|~0-9A-Za-z-]+:\s*[^\r\n]*\r\n", trailer):
                        raise BodyReadError(400, "Malformed chunk trailer")
            chunk = stream.read(size)
            if len(chunk) != size or stream.read(2) != b"\r\n":
                raise BodyReadError(400, "Incomplete chunked request body")
            data.extend(chunk)

    if len(content_lengths) > 1:
        raise BodyReadError(400, "Duplicate Content-Length headers")
    if not content_lengths:
        return b""
    raw = content_lengths[0].strip()
    if not re.fullmatch(r"[0-9]{1,12}", raw):
        raise BodyReadError(400, "Invalid Content-Length")
    size = int(raw)
    if size > limit:
        raise BodyReadError(413, "Request body exceeds size limit")
    body = stream.read(size)
    if len(body) != size:
        raise BodyReadError(400, "Incomplete request body")
    return body
