import requests
from bs4 import BeautifulSoup

def fetch_weather_data(city_codes):
    weather_results = []
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36'
    }

    for city_code in city_codes:
        url = f"http://www.weather.com.cn/weather/{city_code}.shtml"
        response = requests.get(url, headers=headers)
        if response.status_code == 200:
            soup = BeautifulSoup(response.content, 'html.parser')
            weather_list = soup.find('ul', class_="t clearfix")
            weathers = weather_list.find_all('li') if weather_list else []

            for weather in weathers:
                date = weather.find('h1').text
                weather_condition = weather.find('p', class_='wea').text
                temperature = weather.find('p', class_='tem').text.strip()
                wind = weather.find('p', class_='win').find('i').text if weather.find('p', class_='win') else ''

                weather_results.append({
                    'city_code': city_code,
                    'date': date,
                    'weather': weather_condition,
                    'temperature': temperature,
                    'wind': wind
                })
        else:
            print(f"Failed to retrieve data for city code {city_code}")

    return weather_results

if __name__ == "__main__":
    city_codes = ['101010100', '101020100']  # 示例城市代码
    data = fetch_weather_data(city_codes)
    print(data)
