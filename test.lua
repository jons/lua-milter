
banned = {}
banned["149.56.65.240"] = true
banned["125.34.147.77"] = true

function negotiate (envelope, f0, f1, f2, f3)
  major, minor, patch = Milter.version()
  r = Milter.setsymlist(envelope, Milter.SMFIM_HELO, "{client_addr}")
  ret = (Milter.MI_SUCCESS == r and "OK" or "failed")
  --print("[" .. envelope.sid .. "] negotiate " .. major .. "." .. minor .. "." .. patch .. " setsymlist: " .. ret)
  envelope.headers = 0
  envelope.bytes = 0
  return Milter.SMFIS_ALL_OPTS
end

function helo (envelope, identity)
  addr = Milter.getsymval(envelope, "{client_addr}")
  print("[" .. envelope.sid .. "] helo " .. addr .. " \"" .. identity .. "\"")
  if not banned[addr] then
    return Milter.SMFIS_CONTINUE
  end
  -- example multiline reply enabled by libffi:
  r = Milter.setmlreply(envelope, "550", "5.7.1", "IP Blacklisted", "Dude, I build mail servers", "How are you ever going to get in", "Fuck off permanently")
  ret = (Milter.MI_SUCCESS == r and "OK" or "failed")
  print("setmlreply: " .. ret)
  return Milter.SMFIS_REJECT
end

function from (envelope, address, esmtp)
  --print("[" .. envelope.sid .. "] from " .. address)
  envelope.from = address
  envelope.rcpt = {}
  envelope.rcnt = 0
  return Milter.SMFIS_CONTINUE
end

function rcpt (envelope, address, esmtp)
  --print("[" .. envelope.sid .. "] rcpt " .. address)
  table.insert(envelope.rcpt, address)
  envelope.rcnt = envelope.rcnt + 1
  return Milter.SMFIS_CONTINUE
end

function header (envelope, name, value)
  if name then
    if name:lower() == "subject" then
      envelope.subject = value
    end
  end
  envelope.headers = envelope.headers + 1
  return Milter.SMFIS_CONTINUE
end

function data (envelope, segment, len)
  envelope.bytes = envelope.bytes + len
  return Milter.SMFIS_CONTINUE
end

function eom (envelope)
  print("[" .. envelope.sid .. "] eom (" .. envelope.headers .. " headers, " .. envelope.bytes .. " bytes)")
  print("[" .. envelope.sid .. "] " .. (envelope.from or "") .. " : " .. (table.concat(envelope.rcpt, ", ") or "") .. " \"" .. (envelope.subject or "<no-subj>") .. "\"")
  return Milter.SMFIS_CONTINUE
end

function abort (envelope)
  addr = Milter.getsymval(envelope, "{client_addr}")
  print("[" .. envelope.sid .. "] abort " .. (addr or ""))
  return Milter.SMFIS_CONTINUE
end

function close (envelope)
  --print("[" .. envelope.sid .. "] close")
  return Milter.SMFIS_CONTINUE
end


milter = Milter.create()
milter:setConnection("inet:12345")
milter:setFlags(0)
milter:setNegotiate(negotiate)
milter:setHELO(helo)
milter:setENVFROM(from)
milter:setENVRCPT(rcpt)
milter:setHeader(header)
milter:setBody(data)
milter:setEOM(eom)
milter:setAbort(abort)
milter:setClose(close)
print(Milter.SMFIS_CONTINUE, "continue")
print(Milter.SMFIS_REJECT,   "reject")
print(Milter.SMFIS_DISCARD,  "discard")
print(Milter.SMFIS_ACCEPT,   "accept")
print(Milter.SMFIS_TEMPFAIL, "tempfail")
print(Milter.SMFIS_NOREPLY,  "noreply")
print(Milter.SMFIS_SKIP,     "skip")
print(Milter.SMFI_VERSION)
return milter
