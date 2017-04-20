function negotiate (envelope, f0, f1, f2, f3)
  print("[" .. envelope.sid .. "] negotiate")
  r = Milter.setsymlist(envelope, Milter.SMFIM_HELO, "{client_addr}")
  ret = (Milter.MI_SUCCESS == r and "OK" or "failed")
  print("setsymlist: " .. ret)
  envelope.headers = 0
  envelope.bytes = 0
  return Milter.SMFIS_ALL_OPTS
end

function helo (envelope, addr)
  --addr = Milter.getsymval(envelope, "{client_addr}")
  print("[" .. envelope.sid .. "] helo " .. addr)
  return Milter.SMFIS_CONTINUE
end

function header (envelope, name, value)
  envelope.headers = envelope.headers + 1
end

function data (envelope, segment, len)
  envelope.bytes = envelope.bytes + len
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
milter:setClose(close)
print(Milter.SMFIS_CONTINUE, "continue")
print(Milter.SMFIS_REJECT, "reject")
print(Milter.SMFIS_DISCARD, "discard")
print(Milter.SMFIS_ACCEPT, "accept")
print(Milter.SMFIS_TEMPFAIL, "tempfail")
print(Milter.SMFIS_NOREPLY, "noreply")
print(Milter.SMFIS_SKIP, "skip")
return milter
