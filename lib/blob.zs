import { Object } from "core:object";

const MimeTypes = Object.freeze({
    // 🔤 Text
    TEXT_PLAIN: "text/plain",
    TEXT_HTML: "text/html",
    TEXT_CSS: "text/css",
    TEXT_CSV: "text/csv",
    TEXT_XML: "text/xml",
    TEXT_MARKDOWN: "text/markdown",

    // 📦 Application
    JSON: "application/json",
    XML: "application/xml",
    PDF: "application/pdf",
    ZIP: "application/zip",
    GZIP: "application/gzip",
    JAVASCRIPT: "application/javascript",
    OCTET_STREAM: "application/octet-stream",
    FORM_URLENCODED: "application/x-www-form-urlencoded",

    // 📡 Web / APIs
    GRAPHQL: "application/graphql",
    NDJSON: "application/x-ndjson",

    // 🖼️ Images
    PNG: "image/png",
    JPEG: "image/jpeg",
    GIF: "image/gif",
    WEBP: "image/webp",
    SVG: "image/svg+xml",
    BMP: "image/bmp",
    ICO: "image/x-icon",
    TIFF: "image/tiff",

    // 🎵 Audio
    MP3: "audio/mpeg",
    WAV: "audio/wav",
    OGG_AUDIO: "audio/ogg",
    AAC: "audio/aac",
    FLAC: "audio/flac",

    // 🎬 Video
    MP4: "video/mp4",
    WEBM_VIDEO: "video/webm",
    OGG_VIDEO: "video/ogg",
    AVI: "video/x-msvideo",
    MPEG: "video/mpeg",

    // 🧾 Fonts
    WOFF: "font/woff",
    WOFF2: "font/woff2",
    TTF: "font/ttf",
    OTF: "font/otf",

    // 📄 Documents (Office)
    DOC: "application/msword",
    DOCX: "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    XLS: "application/vnd.ms-excel",
    XLSX: "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    PPT: "application/vnd.ms-powerpoint",
    PPTX: "application/vnd.openxmlformats-officedocument.presentationml.presentation",

    // 📱 Misc
    WASM: "application/wasm",
    TAR: "application/x-tar",
    RAR: "application/vnd.rar",
    _7Z: "application/x-7z-compressed"
});