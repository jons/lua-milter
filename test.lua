function negotiate (envelope)
  print("negotiate!")
  return Milter.SMFIS_CONTINUE
end

function helo (envelope)
  print("helo!")
  print(Milter.SMFIS_CONTINUE, "continue")
  print(Milter.SMFIS_REJECT, "reject")
  print(Milter.SMFIS_DISCARD, "discard")
  print(Milter.SMFIS_ACCEPT, "accept")
  print(Milter.SMFIS_TEMPFAIL, "tempfail")
  print(Milter.SMFIS_NOREPLY, "noreply")
  print(Milter.SMFIS_SKIP, "skip")
  return Milter.SMFIS_CONTINUE
end

function close (envelope)
  print(envelope)
  print("close!")
  return Milter.SMFIS_CONTINUE
end

milter = Milter.create()
milter:setConnection("inet:12345")
milter:setFlags(0)
milter:setNegotiate(negotiate)
milter:setHELO(helo)
milter:setClose(close)
return milter
