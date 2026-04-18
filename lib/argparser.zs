import { len } from "core:utf8";
import { format } from "core:io";

const _DIGITS = "0123456789";
const _SHORT = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const _WORD = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";

fn _charInTable(c, table) {
    for (t := 0; t < len(table); t++) {
        if (table[t] == c) {
            return true;
        }
    }
    return false;
}

fn _isDigitChar(c) {
    return _charInTable(c, _DIGITS);
}

/*
 * argv-style CLI parsing for tokens from core:os args().
 *
 * Supports:
 *   --            end-of-options; remaining tokens are positional
 *   --name        boolean flag
 *   --name=val    value (first '=' splits name from value)
 *   --name val    value from next token when option expects a value
 *   -x            short boolean
 *   -xyz          clustered short booleans
 *   -fbar         short option -f with glued value "bar"
 *   -f bar        short option -f with separate value
 *   -             lone dash kept as positional (stdin convention)
 *   -3, -1.5      tokens that look like signed numbers -> positional
 *
 * Unknown flags: stored in result.unknown; if strict is true, errors lists them.
 */

fn strSlice(s, start, end) {
    local out = "";
    for (i := start; i < end; i++) {
        out = out + s[i];
    }
    return out;
}

fn firstEqIndex(s) {
    local n = len(s);
    for (i := 0; i < n; i++) {
        if (s[i] == "=") {
            return i;
        }
    }
    return -1;
}

fn looksLikeSignedNumber(tok) {
    if (tok == null || len(tok) < 2) {
        return false;
    }
    if (tok[0] != "-") {
        return false;
    }
    local n = len(tok);
    local i = 1;
    if (i >= n) {
        return false;
    }
    if (!_isDigitChar(tok[i])) {
        return false;
    }
    i = i + 1;
    while (i < n) {
        if (_isDigitChar(tok[i])) {
            i = i + 1;
            continue;
        }
        if (tok[i] == ".") {
            i = i + 1;
            local sawFrac = false;
            while (i < n) {
                if (_isDigitChar(tok[i])) {
                    sawFrac = true;
                } else {
                    return false;
                }
                i = i + 1;
            }
            return sawFrac;
        }
        return false;
    }
    return true;
}

fn canonicalKey(def) {
    if (def.long != null) {
        return def.long;
    }
    return def.short;
}

fn isValidLongName(name) {
    if (name == null || len(name) == 0) {
        return false;
    }
    for (i := 0; i < len(name); i++) {
        if (!_charInTable(name[i], _WORD)) {
            return false;
        }
    }
    return true;
}

fn isValidShortLetter(ch) {
    if (ch == null || len(ch) != 1) {
        return false;
    }
    return _charInTable(ch[0], _SHORT);
}

class ArgParser {
    fn init() {
        this.strict = false;
        this._defs = [];
        this._longMap = {};
        this._shortMap = {};
    }

    /*
     * Register an option.
     * def fields: long?, short?, takesValue?, multiple?, defaultVal?, help?
     * At least one of long or short is required.
     */
    fn option(def) {
        if (def.long == null && def.short == null) {
            raise "ArgParser.option: need 'long' and/or 'short'";
        }
        if (def.long != null && !isValidLongName(def.long)) {
            raise format("ArgParser.option: invalid long name '{}'", def.long);
        }
        if (def.short != null && !isValidShortLetter(def.short)) {
            raise format("ArgParser.option: invalid short flag '{}'", def.short);
        }
        local entry = {
            long: def.long,
            short: def.short,
            takesValue: def.takesValue == true,
            multiple: def.multiple == true,
            defaultVal: def.defaultVal,
            help: def.help
        };
        if (def.long != null) {
            if (this._longMap[def.long] != null) {
                raise format("ArgParser.option: duplicate long '{}'", def.long);
            }
            this._longMap[def.long] = entry;
        }
        if (def.short != null) {
            if (this._shortMap[def.short] != null) {
                raise format("ArgParser.option: duplicate short '{}'", def.short);
            }
            this._shortMap[def.short] = entry;
        }
        this._defs.push(entry);
        return this;
    }

    fn _findDefLong(name) {
        return this._longMap[name];
    }

    fn _findDefShort(ch) {
        return this._shortMap[ch];
    }

    fn _assignFlag(flags, def, rawVal) {
        local key = canonicalKey(def);
        if (def.multiple) {
            if (flags[key] == null) {
                flags[key] = [];
            }
            flags[key].push(rawVal);
        } else {
            flags[key] = rawVal;
        }
    }

    fn _applyBoolean(flags, def) {
        this._assignFlag(flags, def, true);
    }

    fn _applyValue(flags, def, val) {
        this._assignFlag(flags, def, val);
    }

    fn _consumeValue(argv, i, def, flags, errors) {
        if (i + 1 >= argv.length()) {
            errors.push(format("option '{}' expects a value but argv ended",
                canonicalKey(def)));
            return i;
        }
        local v = argv[i + 1];
        this._applyValue(flags, def, v);
        return i + 1;
    }

    fn _parseLong(tok, argv, i, flags, unknown, errors) {
        local body = strSlice(tok, 2, len(tok));
        local eq = firstEqIndex(body);
        local name = null;
        local inline = null;
        if (eq >= 0) {
            name = strSlice(body, 0, eq);
            inline = strSlice(body, eq + 1, len(body));
        } else {
            name = body;
        }
        local def = this._findDefLong(name);
        if (def == null) {
            if (this.strict) {
                errors.push(format("unknown long option '--{}'", name));
            } else {
                if (inline != null) {
                    unknown[name] = inline;
                } else {
                    unknown[name] = true;
                }
            }
            return i;
        }
        if (def.takesValue) {
            if (inline != null) {
                this._applyValue(flags, def, inline);
                return i;
            }
            return this._consumeValue(argv, i, def, flags, errors);
        }
        if (inline != null) {
            errors.push(format("option '--{}' does not take a value", name));
            return i;
        }
        this._applyBoolean(flags, def);
        return i;
    }

    fn _parseShortCluster(tok, argv, i, flags, unknown, errors) {
        local rest = strSlice(tok, 1, len(tok));
        local j = 0;
        while (j < len(rest)) {
            local letter = strSlice(rest, j, j + 1);
            local def = this._findDefShort(letter);
            if (def == null) {
                if (this.strict) {
                    errors.push(format("unknown short option '-{}'", letter));
                    return i;
                }
                unknown[letter] = true;
                j = j + 1;
                continue;
            }
            if (def.takesValue) {
                local tail = strSlice(rest, j + 1, len(rest));
                if (len(tail) > 0) {
                    this._applyValue(flags, def, tail);
                    return i;
                }
                return this._consumeValue(argv, i, def, flags, errors);
            }
            this._applyBoolean(flags, def);
            j = j + 1;
        }
        return i;
    }

    fn _applyDefaults(flags) {
        this._defs.each(fn(def, idx) {
            local key = canonicalKey(def);
            if (flags[key] == null && def.defaultVal != null) {
                flags[key] = def.defaultVal;
            }
        });
    }

    /* Parse argv (for example args() from core:os). */
    fn parse(argv) {
        local positional = [];
        local flags = {};
        local unknown = {};
        local errors = [];
        local onlyPositional = false;
        local i = 0;
        while (i < argv.length()) {
            local tok = argv[i];
            if (onlyPositional) {
                positional.push(tok);
                i = i + 1;
                continue;
            }
            if (tok == "--") {
                onlyPositional = true;
                i = i + 1;
                continue;
            }
            if (tok == "-") {
                positional.push(tok);
                i = i + 1;
                continue;
            }
            if (len(tok) == 0 || tok[0] != "-") {
                positional.push(tok);
                i = i + 1;
                continue;
            }
            if (looksLikeSignedNumber(tok)) {
                positional.push(tok);
                i = i + 1;
                continue;
            }
            if (len(tok) >= 2 && tok[0] == "-" && tok[1] == "-") {
                local ni = this._parseLong(tok, argv, i, flags, unknown, errors);
                i = ni + 1;
                continue;
            }
            local ni2 = this._parseShortCluster(tok, argv, i, flags, unknown, errors);
            i = ni2 + 1;
        }
        this._applyDefaults(flags);
        local ok = errors.length() == 0;
        return {
            ok: ok,
            errors: errors,
            positional: positional,
            flags: flags,
            unknown: unknown
        };
    }

    fn helpText(programName) {
        if (programName == null) {
            programName = "program";
        }
        local out = format("Usage: {} [options] [--] [positional...]\n", programName);
        this._defs.each(fn(def, idx) {
            local lhs = "";
            if (def.short != null) {
                lhs = lhs + format("-{}", def.short);
            }
            if (def.long != null) {
                if (len(lhs) > 0) {
                    lhs = lhs + ", ";
                }
                lhs = lhs + format("--{}", def.long);
            }
            if (def.takesValue) {
                lhs = lhs + " <value>";
            }
            local h = def.help;
            if (h == null) {
                h = "";
            }
            out = out + format("  {}  {}\n", lhs, h);
        });
        return out;
    }
}

fn createArgParser() {
    return new ArgParser();
}

/* One-shot parse: defs is an array of option objects; argv is args(). */
fn parseWithDefs(defs, argv) {
    local p = new ArgParser();
    defs.each(fn(d, i) {
        p.option(d);
    });
    return p.parse(argv);
}
