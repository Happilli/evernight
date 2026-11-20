package com.safalski.evernight;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.text.Normalizer;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import org.json.JSONArray;
import org.json.JSONObject;

public class CoverLookup {
    private static final String[] BAD = {"karaoke", "tribute", "instrumental", "cover", "lullaby", "piano version"};

    private static String norm(String s) {
        return Normalizer.normalize(s, Normalizer.Form.NFD)
                .replaceAll("\\p{M}+", "")
                .toLowerCase(Locale.ROOT)
                .replaceAll("[^\\p{L}\\p{N}]+", " ")
                .trim();
    }

    private static Set<String> tokens(String s) {
        Set<String> set = new HashSet<>();
        for (String t : norm(s).split(" "))
            if (!t.isEmpty())
                set.add(t);
        return set;
    }

    public static String find(String term) {
        HttpURLConnection c = null;
        try {
            URL u = new URL("https://itunes.apple.com/search?entity=song&limit=10&term="
                    + URLEncoder.encode(term, "UTF-8"));
            c = (HttpURLConnection) u.openConnection();
            c.setConnectTimeout(4000);
            c.setReadTimeout(4000);
            try (InputStream in = c.getInputStream();
                 ByteArrayOutputStream out = new ByteArrayOutputStream()) {
                byte[] buf = new byte[4096];
                int n;
                while ((n = in.read(buf)) > 0)
                    out.write(buf, 0, n);
                JSONArray r = new JSONObject(out.toString("UTF-8")).getJSONArray("results");
                Set<String> want = tokens(term);
                String wantNorm = norm(term);
                double bestScore = 0;
                String best = "";
                for (int i = 0; i < r.length(); i++) {
                    JSONObject o = r.getJSONObject(i);
                    String art = o.optString("artworkUrl100", "");
                    if (art.isEmpty())
                        continue;
                    String hay = o.optString("artistName", "") + " " + o.optString("trackName", "");
                    Set<String> have = tokens(hay);
                    int hit = 0;
                    for (String t : want)
                        if (have.contains(t))
                            hit++;
                    double score = want.isEmpty() ? 0 : (double) hit / want.size();
                    String hayNorm = norm(hay + " " + o.optString("collectionName", ""));
                    for (String b : BAD)
                        if (hayNorm.contains(b) && !wantNorm.contains(b))
                            score -= 0.5;
                    score -= i * 0.01;
                    if (score > bestScore) {
                        bestScore = score;
                        best = art;
                    }
                }
                if (bestScore < 0.6)
                    return "";
                return best.replace("100x100bb", "512x512bb");
            }
        } catch (Exception e) {
            return "";
        } finally {
            if (c != null)
                c.disconnect();
        }
    }
}
