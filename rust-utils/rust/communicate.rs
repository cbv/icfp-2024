use anyhow::anyhow;

const API_KEY_PATH : &str = "API_KEY";

pub fn get_api_token() -> Result<String, std::io::Error> {
    match std::fs::read_to_string(API_KEY_PATH) {
        Ok(s) => Ok(s.trim().into()),
        Err(e) => {
            eprintln!(
                "Failed to read API key. please download an API key from \
                 https://icfpcontest2024.github.io/team.html and save it \
                 in a file named {API_KEY_PATH}.\n");
            Err(e)
        }
    }
}

fn main() -> Result<(), anyhow::Error> {
    let token = get_api_token()?;
    let response = ureq::post("https://boundvariable.space/communicate")
        .set("Authorization", &format!("Bearer {token}"))
        .send_string("S'%4}).$%8")?;
    if response.status() != 200 {
        return Err(anyhow!("non-200 response: {}", response.status()))
    }
    println!("{}", response.into_string()?);
    Ok(())
}
