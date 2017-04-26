function negotiate (envelope, f0, f1, f2, f3)
  major, minor, patch = Milter.version()
  print("[" .. envelope.sid .. "] negotiate " .. major .. "." .. minor .. "." .. patch)
  r = Milter.setsymlist(envelope, Milter.SMFIM_HELO, "{client_addr}")
  ret = (Milter.MI_SUCCESS == r and "OK" or "failed")
  print("setsymlist: " .. ret)
  envelope.headers = 0
  envelope.bytes = 0
  return Milter.SMFIS_ALL_OPTS
end

function helo (envelope, identity)
  addr = Milter.getsymval(envelope, "{client_addr}")
  print("[" .. envelope.sid .. "] helo " .. addr .. "\"" .. identity .. "\"")
  return Milter.SMFIS_CONTINUE
  -- example multiline reply enabled by libffi:
  --r = Milter.setmlreply(envelope, "421", "4.4.5", "We're up to something here...", "Come back later.")
  --ret = (Milter.MI_SUCCESS == r and "OK" or "failed")
  --print("setmlreply: " .. ret)
  --return Milter.SMFIS_TEMPFAIL
end

function header (envelope, name, value)
  envelope.headers = envelope.headers + 1
  return Milter.SMFIS_CONTINUE
end

function data (envelope, segment, len)
  envelope.bytes = envelope.bytes + len
  return Milter.SMFIS_CONTINUE
end

function abort (envelope)
  print("[" .. envelope.sid .. "] abort")
  return Milter.SMFIS_CONTINUE
end

function close (envelope)
  print("[" .. envelope.sid .. "] close (" .. envelope.headers .. " headers, " .. envelope.bytes .. " bytes)")
  return Milter.SMFIS_CONTINUE
end


milter = Milter.create()
milter:setConnection("inet:12345")
milter:setFlags(0)
milter:setNegotiate(negotiate)
milter:setHELO(helo)
milter:setHeader(header)
milter:setBody(data)
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
